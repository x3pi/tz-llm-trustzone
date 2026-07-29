cmd_drivers/tzdriver/whitelist/modules.order := {  :; } | awk '!x[$$0]++' - > drivers/tzdriver/whitelist/modules.order
