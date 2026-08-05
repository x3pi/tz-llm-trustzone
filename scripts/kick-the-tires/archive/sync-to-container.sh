#!/bin/bash
# The Docker builder container (tzllm_fixed_builder) has its bind mounts
# baked in at the OLD tz-llm-ae/ host paths from before this project/
# restructuring (recreating the container to point at project/ instead
# would need `docker commit` + recreate, which needs ~11GB free disk this
# host does not reliably have -- see STATUS.md "Container bind-mount trap").
#
# Until that's done, any source edit under project/tz-llm/ has ZERO effect
# on what the container builds unless it's synced here first. Run this
# before every build-oh-docker.sh invocation that follows a source edit.
set -e
cd "$(dirname "$0")/../.."
OLD=/mnt/2d4726e7-046b-47c7-b9a9-d2a9cc0cfc8d/Work/OPTEE/tz-llm-ae/tz-llm

for d in tee_os_kernel linux-5.10-opi tzdriver; do
    echo "=== syncing tz-llm/$d -> tz-llm-ae/tz-llm/$d ==="
    rsync -a --delete "tz-llm/$d/" "$OLD/$d/" 2>&1 | grep -v "Operation not permitted" || true
done
echo "Done. (Permission-denied warnings on root-owned files deep in the"
echo "vendored musl-libc submodule are expected and harmless -- those files"
echo "are identical either way and irrelevant to project source changes.)"
