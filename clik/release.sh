#! /bin/bash

if [ "$#" -ne 1 ]; then
    echo "usage: $0 <clik_release checkout directory>"
    exit 1
fi

RELEASE_DIR=$1
RELEASE_COMMIT_FILE="${RELEASE_DIR}/.release-commit"
SOURCE_GIT_DIR=$(git rev-parse --absolute-git-dir)
SOURCE_COMMIT=$(git rev-parse HEAD)
PATCH_NAME=.release.patch

if [ ! -f ${RELEASE_COMMIT_FILE} ]; then
  echo "Initial release: ${SOURCE_COMMIT}"
  # Export all files except ignored, untracked and export-ignore files to the clik_release checkout.
  git archive --format=tar HEAD | tar x -C ${RELEASE_DIR} || exit 1
  # Add all exported files to the clik_release checkout.
  cd ${RELEASE_DIR}
  git add -A
else
  PREV_COMMIT=$(cat ${RELEASE_COMMIT_FILE})
  echo "Incremental release: ${PREV_COMMIT} .. ${SOURCE_COMMIT}"
  cd ${RELEASE_DIR}
  # Generate a diff between the clik_release checkout and the current revision.
  git --git-dir ${SOURCE_GIT_DIR} diff ${PREV_COMMIT} HEAD > ${PATCH_NAME}
  # Apply this diff to the clik_release checkout.
  git apply --exclude=.gitattributes --exclude=.gitignore --exclude=.gitmodules \
    --exclude=external/hal --exclude=release.sh ${PATCH_NAME} || exit 1
  git add -A
fi

# Update the 'last release' commit ID.
echo -n "${SOURCE_COMMIT}" > "${RELEASE_COMMIT_FILE}"
git add "${RELEASE_COMMIT_FILE}"

echo
echo "Run 'git commit' to finish the release."
