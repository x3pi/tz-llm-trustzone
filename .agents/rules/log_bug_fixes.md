# Rule: Log Bug Fixes to error-build-xpain-evm.md

Whenever making bug fixes, logic corrections, or architecture updates to the EVM-TA or Xapian integration in this repository:
1. You MUST update `/home/abc/nhat/tz-llm-trustzone/tz-llm/evm-tee/evm-ta/error-build-xpain-evm.md`.
2. Document:
   - Error title & Category (Build-time / Runtime / Logic / Persistence)
   - Symptoms (UART error, revert reason, syscall crash, etc.)
   - Affected file paths
   - Technical root cause explanation
   - Applied fix and code changes
