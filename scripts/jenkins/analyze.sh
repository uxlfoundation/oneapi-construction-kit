#!/bin/bash -ex

MODE="clang"
BUILD_DIR="build/"
LLVM_INSTALL_DIR="llvm_install/"
LOG="analyze.log"
CPPCHECK="cppcheck"
OCLINT="oclint"
CLANGPP="clang++"
CLANG="clang"
SCANBUILD="scan-build"
[[ "$WORKSPACE" == "" ]] && WORKSPACE="./" # Pre-defined on Jenkins.

# This function is not required for this script to 'die', because it is running
# -ex any bad exitcode will cause it to die.  This just provides a way of
# providing an error message.
function die {
  echo
  echo "ERROR: $*" >&2
  exit 1
}

function print_usage() {
  set +x
  echo "Usage: $(basename $0) [OPTION]*"
  echo
  echo "ANALYZERS (set via '-a'):"
  echo "  clang             Run the clang static analyzer."
  echo "  cppcheck          Run the cppcheck analyzer/linter."
  echo "  oclint            Run the oclint analyzer/linter."
  echo
  echo "OPTION (current/default value in brackets):"
  echo "  -a <analyzer>     Select an analyzer from list above ($MODE)."
  echo "  -h                Print usage."
  echo "  -l <log_file>     Where to write the log ($LOG)."
  echo "  -n <build_dir>    Directory with pre-generated headers ($BUILD_DIR)."
  echo "  -i <install_dir>  Directory with LLVM installed ($LLVM_INSTALL_DIR)."
  echo "  -w <workspace>    Workspace directory ($WORKSPACE)."
  echo "  -0 <clang>        Name or path for clang binary ($CLANG)."
  echo "  -1 <clang++>      Name or path for clang++ binary ($CLANGPP)."
  echo "  -2 <scan-build>   Name or path for scan-build binary ($SCANBUILD)."
  echo "  -3 <cppcheck>     Name or path for cppcheck binary ($CPPCHECK)."
  echo "  -4 <oclint>       Name or path for oclint binary ($OCLINT)."
}

function parse_opts {
  while getopts ":a:hl:n:i:w:0:1:2:3:4:" OPTION ; do
    case $OPTION in
      a) MODE="$OPTARG" ;;
      h) print_usage ; exit 0 ;;
      l) LOG="$OPTARG" ;;
      n) BUILD_DIR="$OPTARG" ;;
      i) LLVM_INSTALL_DIR="$OPTARG" ;;
      w) WORKSPACE="$OPTARG" ;;
      0) CLANG="$OPTARG" ;;
      1) CLANGPP="$OPTARG" ;;
      2) SCANBUILD="$OPTARG" ;;
      3) CPPCHECK="$OPTARG" ;;
      4) OCLINT="$OPTARG" ;;
      \?) print_usage ; die "Invalid option: -$OPTARG" ;;
    esac
  done
}

parse_opts "$@"
cd "$WORKSPACE"

function check_not_exists {
  local CHECK="$1" ; shift
  [ ! -e "$CHECK" ] || die "'$CHECK' already exists.  $*"
}

function check_exists {
  local CHECK="$1" ; shift
  [ -e "$CHECK" ] || die "'$CHECK' does not exist.  $*"
}

function check_dir {
  local CHECK="$1" ; shift
  check_exists "$CHECK" "$*"
  [ -d "$CHECK" ] || die "'$CHECK' exists but is not a directory.  $*"
}

function check_bin {
  local CHECK="$1"
  which "$CHECK" &> /dev/null || die "Could not find binary '${CHECK}'."
}

function create_dir {
  local DIR="$1" ; shift
  check_not_exists "$DIR" "$*"
  mkdir "$DIR"
}

function include_dirs {
  for x in $PREBUILD_INCLUDE_DIRS $EXTRA_INCLUDE_DIRS ; do
    echo "$@ $x"
  done
}

PREBUILD_INCLUDE_DIRS=" \
    source/ \
    modules/api/include/ \
    modules/cargo/include/ \
    modules/compiler/include/ \
    modules/core/include/ \
    modules/extension/include/ \
    modules/host/include/ \
    modules/vecz/include/ \
    include/ \
    modules/builtins/include/builtins/ \
    modules/builtins/abacus/include/ \
    modules/builtins/libimg/include/ \
"
for dir in $PREBUILD_INCLUDE_DIRS ; do
  check_dir "$dir" "Is WORKSPACE wrong?  It is '$WORKSPACE'."
done

EXTRA_INCLUDE_DIRS=" \
    $BUILD_DIR/include/ \
    $LLVM_INSTALL_DIR/include/ \
"
if [[ "$MODE" != "clang" ]] ; then
  check_dir "$BUILD_DIR" "It is required for pre-generated headers."
  check_dir "$LLVM_INSTALL_DIR" "It is required for LLVM/Clang headers."
  for dir in $EXTRA_INCLUDE_DIRS ; do
    check_dir "$dir" "You need to build LLVM and ComputeAorta to generate the headers."
  done
fi

SOURCE_DIRS="\
    source/ \
    modules/api/source/ \
    modules/compiler/source/ \
    modules/core/source/ \
    modules/host/source/ \
    modules/extension/source/ \
    modules/vecz/source/ \
    modules/builtins/source/ \
    modules/builtins/abacus/source/abacus_cast/ \
    modules/builtins/abacus/source/abacus_common/ \
    modules/builtins/abacus/source/abacus_geometric/ \
    modules/builtins/abacus/source/abacus_integer/ \
    modules/builtins/abacus/source/abacus_math/ \
    modules/builtins/abacus/source/abacus_memory/ \
    modules/builtins/abacus/source/abacus_misc/ \
    modules/builtins/abacus/source/abacus_relational/ \
    modules/builtins/libimg/source/ \
"

# The Aorta target does not currently compile, so not analyzable.
SOURCE_FILES="$(find $SOURCE_DIRS -maxdepth 1 -name '*.cpp')"

function run_cppcheck {
  check_bin "$CPPCHECK"

  local LANGOPTS="--std=c++11 --quiet -j8"
  local SELFCHECK="--enable=missingInclude --check-config --suppress=missingIncludeSystem"
  local CODECHECK="--force --enable=warning,style,performance,portability,information"
  local INCLUDES="`include_dirs -I`"

  # cppcheck can't reason about certain LLVM ifdefs.
  local DISABLE="-ULLVM_NATIVE_ASMPARSER -ULLVM_NATIVE_DISASSEMBLER"

  # This test is buggy, it is producing warnings that are wrong (saying that
  # member class variables are being initialized with their own values).
  DISABLE+=" --suppress=selfInitialization"

  # This test has a small number of false positives related to template classes
  # (where only some template instantiations have a certain member variable,
  # but the checker expected all instantiations to initialize it).  There are
  # also a large number of genuine triggers.
  DISABLE+=" --suppress=uninitMemberVar"

  # Unfortunately this test has some false positives that sounds good, but then
  # you waste time figuring out why its suggestions are broken.  Simple
  # example: it doesn't know that std::initializer_list is light weight enough
  # to pass by value.  Complex example: It suggests passing format strings by
  # reference, but those strings are then used by va_start, so they should be
  # passed by value.
  DISABLE+=" --suppress=passedByValue"

  # This doesn't trigger much, but the builtins library genuinely needs to do
  # some unsafe pointer casts.
  DISABLE+=" --suppress=invalidPointerCast"

  # This is purely a style issue, about variables that could be placed in a
  # tighter scope.  We have "violations", so disable.
  DISABLE+=" --suppress=variableScope"

  # cppcheck considers all implicit constructors to be a problem.  Sometimes
  # they're not, unfortunately C++ doesn't have an 'implicit' keyword.
  DISABLE+=" --suppress=noExplicitConstructor"

  $CPPCHECK --version 2>&1 | tee "$LOG"

  echo -e "\n\n\n* Self-check cppcheck options." | tee -a "$LOG"
  time $CPPCHECK $LANGOPTS $INCLUDES $CODECHECK $SELFCHECK $SOURCE_FILES 2>&1 | tee -a "$LOG"

  echo -e "\n\n\n* Run cppcheck." | tee -a "$LOG"
  time $CPPCHECK $LANGOPTS $INCLUDES $CODECHECK $DISABLE $SOURCE_FILES 2>&1 | tee -a "$LOG"
}

function run_oclint {
    check_bin "$OCLINT"

    local LANGOPTS="-c --std=c++11"

    # Set some defines required for building various components.  There are
    # quite a lot of these now, should probably use a compilation database.
    LANGOPTS+=" -D__SPIR64__ -D__STDC_CONSTANT_MACROS -D__STDC_LIMIT_MACROS"

    # We're using headers from an Core build, work around other architectures.
    LANGOPTS+=" -DkTargetKind_ARM=kTargetKind_Core"
    LANGOPTS+=" -DkTargetKind_Mips=kTargetKind_Core"
    LANGOPTS+=" -DkTargetKind_X86=kTargetKind_Core"

    local CODECHECK="-enable-global-analysis"

    # This slows down analysis a lot, and there is a separate path in this
    # script to actually use the clang static analyzer directly, but that path
    # cannot analyze the builtins.  Enabling this flag is the only current way
    # to statically analyze the builtins library, though it can only look at the
    # C++ parts.
    CODECHECK+=" -enable-clang-static-analyzer"

    local INCLUDES="`include_dirs -I`"
    local DISABLE=""

    # This seems to be triggering an enormous number of false positives in
    # macros used by the vectorizer, but I can't figure out why, so disabling.
    DISABLE+=" -disable-rule=DeadCode"

    # Too many hits of this rule, so disabling for now, but we should really
    # follow this rule (sometimes we do, sometimes we don't).
    DISABLE+=" -disable-rule=InvertedLogic"

    # OCLint doesn't seem to understand the noreturn attribute, so this is
    # showing up false positives.
    DISABLE+=" -disable-rule=MissingBreakInSwitchStatement"

    # This is purely a formatting issue, so ignoring it for now.
    DISABLE+=" -disable-rule=UseEarlyExitsAndContinue"
    DISABLE+=" -disable-rule=CollapsibleIfStatements"
    DISABLE+=" -disable-rule=UnnecessaryElseStatement"

    # Leaving this disabled for now, there are no genuine false positives, but
    # we have switches of switches, with an abort at the end, the current form
    # is much more readable than adding defaults just to satisfy the tool.
    DISABLE+=" -disable-rule=SwitchStatementsShouldHaveDefault"

    # I don't know why this is a rule.  A common form for us to the have a loop
    # searching for something and return (i.e. branch) when we find it.
    DISABLE+=" -disable-rule=AvoidBranchingStatementAsLastInLoop"

    # I checked out a subset of the lines this was warning about, in my opinion
    # the brackets helped readability, even if they were technically "useless".
    DISABLE+=" -disable-rule=UselessParentheses"

    # Not sure about this test, we have default cases in switch statements that
    # are provably dead-code (the enum is fully covered), but leaving them
    # there protects against future cases where the enum is extended.
    DISABLE+=" -disable-rule=SwitchStatementsDon'TNeedDefaultWhenFullyCovered"

    # We don't need a tool complaining about the length of variable names.
    DISABLE+=" -disable-rule=LongVariableName"
    DISABLE+=" -disable-rule=ShortVariableName"

    # We'll get formatting sorted via clang-format, this is noise.
    DISABLE+=" -disable-rule=LongLine"

    # The rule is sensible enough, but OpenCL API functions violate it, so it's
    # a non-starter for us.
    DISABLE+=" -disable-rule=TooManyParameters"

    # This is just a style guideline, that we don't follow.
    DISABLE+=" -disable-rule=DefaultLabelNotLastInSwitchStatement"

    # Another style guideline, some of the switches that trigger may get more
    # cases added in the future.
    DISABLE+=" -disable-rule=TooFewBranchesInSwitchStatement"

    # Unfortunately we have to disable this rule.  It doesn't hurt us much
    # because Clang/GCC already has this warning and seem to do a better job of
    # it.  This warns on RAII scope variables, such as locks, that are not
    # "used", but their creation/destruction has an effect.  So disable.
    DISABLE+=" -disable-rule=UnusedLocalVariable"
    DISABLE+=" -disable-rule=UnusedMethodParameter"

    # This rule is flawed, it considers writes to a non-const reference
    # parameter to be a "parameter reassignment", we're using this to return
    # values in many cases.
    DISABLE+=" -disable-rule=ParameterReassignment"

    # OpenCL uses bitfields, we check those bitfields in conditional
    # statements, so turn this rule off.
    DISABLE+=" -disable-rule=BitwiseOperatorInConditional"

    # OCLint considers private static members to be bad design, I'm not sure
    # that I agree, and we have many violations.
    DISABLE+=" -disable-rule=AvoidPrivateStaticMembers"

    # We have some huge violations of these "rules", and many many minor
    # violations.  Disabling for now, but we should consider whether to
    # refactor to avoid these violations.
    DISABLE+=" -disable-rule=HighNpathComplexity"
    DISABLE+=" -disable-rule=HighCyclomaticComplexity"
    DISABLE+=" -disable-rule=DeepNestedBlock"

    # We also violate these rules.  It's about lines of comments vs lines of
    # code.  We should probably obey these rules, but Doxygen is higher
    # priority.
    DISABLE+=" -disable-rule=HighNcssMethod"
    DISABLE+=" -disable-rule=LongMethod"

    $OCLINT -version 2>&1 | tee "$LOG"

    echo -e "\n\n\n* Run oclint on sources." | tee -a "$LOG"
    time $OCLINT \
        $CODECHECK $DISABLE \
        $SOURCE_FILES -- $LANGOPTS $INCLUDES \
        2>&1 | tee -a "$LOG"
}

# Note that unlike the oclint/cppcheck analyses, this analysis by the clang
# static analyzer does not consider the builtins library because that gets
# compiled by the in-tree clang (and thus bypasses scan-build's attempts to
# hook into the compilation process).  The oclint analysis can run the clang
# static analyzer on the C++ parts of the builtins library though.
function run_clang_static_analyzer {
  check_bin "$CLANG"
  check_bin "$CLANGPP"
  check_bin "$SCANBUILD"
  check_bin "/usr/share/clang/$(basename $SCANBUILD)/libexec/c++-analyzer"
  check_bin "/usr/share/clang/$(basename $SCANBUILD)/libexec/ccc-analyzer"

  local CLANG_BUILD="${BUILD_DIR%/}_clangsa/"
  create_dir "$CLANG_BUILD" "Cowardly refusing to overwrite previous build."
  cd "$CLANG_BUILD"

  # Doing a debug build gives much clearer results because the analysis can use
  # asserts to eliminate false positives.
  export CCC_CC="$CLANG"
  export CCC_CXX="$CLANGPP"
  time cmake \
    -DOCL_LLVM_INSTALL_DIR=$LLVM_INSTALL_DIR \
    -DCA_BUILTINS_TOOLS_DIR=$LLVM_INSTALL_DIR/bin/ \
    -DCMAKE_BUILD_TYPE:STRING=Debug \
    -DCMAKE_CXX_COMPILER:STRING=/usr/share/clang/$SCANBUILD/libexec/c++-analyzer \
    -DCMAKE_C_COMPILER:STRING=/usr/share/clang/$SCANBUILD/libexec/ccc-analyzer \
    ..

  # Only build specific targets so as we can avoid building targets with a lot
  # of noise (e.g. the CTS).
  time $SCANBUILD make -j8 CL UnitCL Codeplay_kts \
    2>&1 | tee "$LOG"

  rm -rf "$CLANG_BUILD"
  echo "If you're running locally then scan-view is the best way of viewing "
  echo "the results."
}

echo "MODE: $MODE"
echo "BUILD_DIR: $BUILD_DIR"
echo "LLVM_INSTALL_DIR: $LLVM_INSTALL_DIR"
echo "LOG: $LOG"

if [[ "cppcheck" == "$MODE" ]] ; then
  echo "CPPCHECK: $CPPCHECK"
  run_cppcheck
elif [[ "oclint" == "$MODE" ]] ; then
  echo "OCLINT: $OCLINT"
  run_oclint
elif [[ "clang" == "$MODE" ]] ; then
  echo "CLANGPP: $CLANGPP"
  echo "CLANG: $CLANG"
  echo "SCANBUILD: $SCANBUILD"
  run_clang_static_analyzer
else
  die "Unrecognised mode '$MODE'"
fi

echo "SCRIPT REACHED END"
