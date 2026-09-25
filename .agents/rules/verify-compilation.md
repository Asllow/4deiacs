---
trigger: always_on
---

# ESP-IDF Build Verification Rule

## Mandatory

After EVERY code change, the agent MUST validate the complete ESP-IDF build before considering the task finished.

### Required sequence

From the project root, the agent MUST execute the following commands in this exact order:

idf.py fullclean

idf.py build

The agent MUST:

1. Execute `idf.py fullclean`.
2. Verify that `idf.py fullclean` completed successfully.
3. Execute `idf.py build`.
4. Verify the actual process exit status of the build.
5. If the build fails, inspect the compiler and linker errors.
6. Determine whether the failure was caused by the agent's changes.
7. If the failure was caused by the agent's changes, fix the problem.
8. After making a correction, execute the complete validation sequence again:
   - `idf.py fullclean`
   - `idf.py build`
9. Repeat this process until the build succeeds or an external blocker prevents validation.

## Final Validation Requirement

The task MUST NOT be considered complete until the final version of the modified code has successfully passed:

`idf.py fullclean`

followed by:

`idf.py build`

The build must be performed using the actual ESP-IDF environment configured for the project.

The agent MUST NOT consider static code analysis, syntax inspection, IDE diagnostics, or previous build results sufficient to satisfy this requirement.

## Prohibited Behavior

The agent MUST NOT:

- Assume that the code compiles without actually running the build.
- Claim that the project compiles without executing `idf.py build`.
- Perform only an incremental build when the final validation is required.
- Skip `idf.py fullclean`.
- Declare the task complete while the final build is failing.
- Ignore compiler errors.
- Ignore linker errors.
- Ignore ESP-IDF configuration or component errors.
- Treat warnings as errors unless the project's build configuration explicitly does so.
- Report a successful build based only on the absence of visible errors in the editor or IDE.
- Stop after modifying code without performing the required final validation.

## Environment

Before running the commands, the agent MUST ensure that it is operating in the correct project directory and using the ESP-IDF environment configured for the project.

If the project requires a specific ESP-IDF version, Python virtual environment, target, SDK configuration, or other build environment, the agent MUST use the project's configured environment.

The agent MUST NOT arbitrarily change the project's ESP-IDF version or build configuration merely to make the build pass.

## Build Failure Workflow

When the build fails, follow this procedure:

1. Read the complete relevant compiler output.
2. Identify the first meaningful error.
3. Trace the error back to its source.
4. Determine whether the error is related to the current changes.
5. Apply the appropriate correction.
6. Run:

`idf.py fullclean`

7. Then run:

`idf.py build`

8. Verify the result.
9. Repeat if necessary.

The agent should fix the root cause instead of hiding, suppressing, or bypassing the error.

## External Blockers

If compilation cannot be performed because of an external problem, such as:

- ESP-IDF not installed;
- required toolchain unavailable;
- missing Python environment;
- missing project dependency;
- unavailable hardware when hardware is genuinely required for compilation;
- terminal unavailable;
- permission failure;
- corrupted external environment;
- network dependency unavailable when required by the build;

the agent MUST explicitly report that the ESP-IDF build could not be verified.

The agent MUST NOT claim that the build passed.

## Required Final Report

After successful validation, the agent MUST report:

ESP-IDF Build: PASS
Full Clean: PASS
Build: PASS

If the build could not be validated, the agent MUST report:

ESP-IDF Build: NOT VERIFIED
Reason: <specific reason>

If the build failed after all reasonable corrections, the agent MUST report:

ESP-IDF Build: FAIL
Full Clean: <PASS/FAIL>
Build: FAIL
Error: <specific relevant error>

## Rule Priority

This verification is a mandatory final step for every task that modifies project code.

No code modification task is considered finished until the final ESP-IDF validation has been performed.

The final validation must always follow this sequence:

`idf.py fullclean`

then

`idf.py build`

Only a successful execution of both commands satisfies this rule.
