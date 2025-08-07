# oneAPI Construction Kit CI Overview

## Workflow definition: listing & types

### Listing

`CodeQL`: codeql.yml
- description: runs the CodeQL tool

`create llvm artefacts`:
create_llvm_artefacts.yml
- description: creates llvm artefacts

`Build and Package`: create_publish_artifacts.yml
- description: builds and packages publish artefacts

`Build documentation`: docs.yml
- description: builds docs for PR testing

`Run planned testing`: planned_testing_caller.yml
- description: runs planned_testing-style tests, called from an llvm version caller

`run planned tests for llvm 19`: planned_testing_caller_19.yml
- description: runs planned_tests for llvm 19

`run planned tests for llvm 20`: planned_testing_caller_20.yml
- description: runs planned_tests for llvm 20

`run planned tests for llvm 21`: planned_testing_caller_21.yml
- description: runs planned_tests for llvm 21

`run full planned tests for experimental llvm main`: planned_testing_caller_main.yml
- description: runs planned_tests for experimental llvm main

`run limited planned tests for experimental llvm main`: planned_testing_caller_mini_main.yml
- description: runs limited planned_tests for experimental llvm main

`Seed the cache for ock builds`: pr_tests_cache.yml
- description: seeds the cache for OCK builds

`publish docker images`: publish_docker_images.yml 
- description: builds and publishes docker images

`Run external tests`: run_ock_external_tests.yml
- description: runs external OCK tests

`Run ock internal tests`: run_ock_internal_tests.yml
- description: runs internal OCK tests

`Run ock tests for PR style testing`: run_pr_tests_caller.yml
- description: runs PR-style tests

`Scorecard supply-chain security`: scorecard.yml
- description: runs scorecard analysis and reporting

`Create a cache OpenCL-CTS artefact`: create_opencl_cts_artefact.yml
- description: Workflow for creating and caching OpenCL-CTS artefact

### `schedule:` workflows

`CodeQL`: codeql.yml

`run planned tests for llvm 19`: planned_testing_caller_19.yml

`run planned tests for llvm 20`: planned_testing_caller_20.yml

`run planned tests for llvm 21`: planned_testing_caller_21.yml

`run full planned tests for experimental llvm main`: planned_testing_caller_main.yml

`run limited planned tests for experimental llvm main`: planned_testing_caller_mini_main.yml

`Scorecard supply-chain security`: scorecard.yml

### `workflow_dispatch:` workflows (manually runnable and available in forks)

`run planned tests for llvm 19`: planned_testing_caller_19.yml

`run planned tests for llvm 20`: planned_testing_caller_20.yml

`run planned tests for llvm 21`: planned_testing_caller_21.yml

`Seed the cache for ock builds`: pr_tests_cache.yml

`Build and Package`: create_publish_artifacts.yml

`Build documentation`: docs.yml

`Run ock internal tests`: run_ock_internal_tests.yml

`Create a cache OpenCL-CTS artefact`: create_opencl_cts_artefact.yml

### PR workflow

`Run ock internal tests`: run_ock_internal_tests.yml

## Docker: images, dockerfiles and workflow
### Container images
oneAPI Construction Kit CI container images can be found under the [uxlfoundation repo packages tab](https://github.com/orgs/uxlfoundation/packages):
```
       ock_ubuntu_22.04-x86-64:latest
       ock_ubuntu_22.04-aarch64:latest
       ock_ubuntu_24.04-x86-64:latest
```

### Dockerfiles and container build workflow
Corresponding dockerfiles used to build the above container images can be found in the repo [dockerfile folder](https://github.com/uxlfoundation/oneapi-construction-kit/tree/main/.github/dockerfiles):
```
       Dockerfile_22.04-x86-64
       Dockerfile_22.04-aarch64
       Dockerfile_24.04-x86-64
```
The `publish docker images` workflow is configured to rebuild the containers when any dockerfile update is pushed to main.

## LLVM artefact management

Planned_testing workflows each use a particular llvm artefact according to llvm version, OS and architecture (e.g. llvm 19/20/21, Ubuntu_24, x86_64). The specific version to use and branch to reference are contained in the .yml workflow definition. llvm artefacts can be installed, built or accessed as pre-built artefacts from Github cache. The default behavour is to use a cached version where available with the cached version being built if it does not exist.

Support for future llvm artefact versions can be added by copying the latest planned_testing workflow definition and updating the llvm variables accordingly (e.g. 21 to 22).

## Adding Self-Hosted Runners
oneAPI Construction Kit CI runs on standard Github runners. References to these runners can be replaced by references to self-hosted runners by updating individual `runs-on:` settings in the CI config to use an appropriate self-hosted runner string.

Further information on deploying self-hosted runners can be found in the [github docs](https://docs.github.com/en/actions/concepts/runners/self-hosted-runners).
Further information on `runs-on:` can also be found in the [github docs](https://docs.github.com/en/actions/reference/workflows-and-actions/workflow-syntax#jobsjob_idruns-on).

## Setting up PR testing workflow pre-requisites
A number of individual PR test jobs in the `Run ock internal tests` (PR testing) workflow include a testing phase which involves calling the `run_cities.py` script to execute a portion of the tests. This phase also requires the pre-built `opencl_cts_host_x86_64_linux` opencl_cts artifact which must be available in repo cache prior to running these tests. If this artifact is not provided in the cache the PR tests workflow will fail.

The opencl_cts artifact concerned can be built and cached by calling the `Create a cache OpenCL-CTS artifact` workflow from the web interface (i.e. via a `workflow_dispatch:` manual event trigger) in advance of running the PR tests. This workflow can be called in forks.
There are a number of inputs to this workflow which relate to Git checkout references in OpenCL repos. The default values for these at time of writing are:
```
      header_ref:
        description: 'Git checkout ref for OpenCL Headers repo'
        default: 'v2025.06.13'
      icd_loader_ref:
        description: 'Git checkout ref for OpenCL ICD Loader repo'
        default: 'v2024.10.24'
      opencl_cts_ref:
        description: 'Git checkout ref for OpenCL-CTS repo'
        default: 'v2025-04-21-00'
```
These default values can also be set interactively on a per-run basis when called from the web interface. 

At the point at which an update to the opencl_cts cache artifact is required (e.g. when new Git checkout references are available and the workflow inputs default values shown above have been updated accordingly) the existing artifact should be manually deleted prior to re-running the artefact creation workflow. The update workflow will fail if an existing cached artifact is found. Consideration should be given to avoid impacting any in-progress PRs referencing the previous opencl_cts cache artefact version.

## Running planned_testing workflows
### Planned_testing workflows in forks
Planned_testing workflows are configured to run via `workflow_dispatch:` (manual event trigger) in forks. Examples can be found [in this fork](https://github.com/AERO-Project-EU/oneapi-construction-kit/actions?query=event%3Aworkflow_dispatch).

### Tailoring planned_testing workflows
The following planned_testing workflows call `Run planned testing` (planned_testing_caller.yml) as a sub-workflow:
```
      run planned tests for llvm 19: planned_testing_caller_19.yml
      run planned tests for llvm 20: planned_testing_caller_20.yml
      run planned tests for llvm 21: planned_testing_caller_21.yml
```
They can be tailored to run specific llvm versions (e.g. 19), target lists (e.g. host_x86_64_linux) and test options (e.g. test_sanitizers), etc., by setting the `inputs:` values to `Run planned testing` accordingly. See the planned_testing workflow .yml files for examples of current default values and tailoring options. Note that tailored values should be set directly in the workflow config and cannot currently be updated interactively on a per-run basis when called from the web interface. 
