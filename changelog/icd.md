Upgrade guidance:
* The submodule `source/cl/external/OpenCL-ICD-Loader` has been removed and
  replaced by cmake fetchcontent logic to fetch it from github. It can built as
  before, except that the registry setting for the ICD needs to be set on
  windows. The script icd-register.ps1 can be used for this, although
  administrator privileges will be required.
