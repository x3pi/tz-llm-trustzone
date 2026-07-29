cmd_scripts/resource_tool := gcc -Wp,-MMD,scripts/.resource_tool.d -Wall -Wmissing-prototypes -Wstrict-prototypes -O2 -fomit-frame-pointer -std=gnu89         -o scripts/resource_tool scripts/resource_tool.c   

source_scripts/resource_tool := scripts/resource_tool.c

deps_scripts/resource_tool := \

scripts/resource_tool: $(deps_scripts/resource_tool)

$(deps_scripts/resource_tool):
