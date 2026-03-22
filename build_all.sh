#!/bin/bash
# Build all board environments and report results

ENVS="m5stickcplus_11 m5stickcplus_2 m5_cardputer m5_cardputer_adv t_lora_pager t_display_16mb diy_smoochie"
PASSED=""
FAILED=""

for env in $ENVS; do
  echo "========== Building $env =========="
  if pio run -e "$env"; then
    PASSED="$PASSED $env"
  else
    FAILED="$FAILED $env"
  fi
  echo ""
done

echo "======================================"
echo "RESULTS:"
echo "  PASSED:$PASSED"
if [ -n "$FAILED" ]; then
  echo "  FAILED:$FAILED"
  exit 1
else
  echo "  All builds passed!"
fi
