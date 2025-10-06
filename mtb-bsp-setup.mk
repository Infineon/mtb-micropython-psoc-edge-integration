MTB_LIBS_DIR ?= $(TOP)/lib/mtb-psoc-edge-libs

# This file is used to track the current active bsp in 
# the ModusToolbox library project.
# This prevents the need to specify BOARD=XXX
# variable for every call to make in the micropython build. 
# Instead, it only needs to be specified for the first build:
# $ make BOARD=KIT_PSE84_AI 	//first time build
# $ make 						//subsequent builds (without BOARD variable)
MTB_ACTIVE_BSP_FILE = $(MTB_LIBS_DIR)/.mtb_active_bsp

# Check if the active BSP file exists and read its content
ifneq ($(wildcard $(MTB_ACTIVE_BSP_FILE)),)
    ACTIVE_BOARD := $(shell cat $(MTB_ACTIVE_BSP_FILE) 2>/dev/null | head -1)
else
    ACTIVE_BOARD :=
    $(info No active BSP file found)
endif

# Use active board if no BOARD is specified.
ifeq ($(BOARD),)
    ifneq ($(ACTIVE_BOARD),)
        BOARD := $(ACTIVE_BOARD)
        $(info Using active board: $(BOARD))
    endif
endif

BOARD_DIR  = boards/$(BOARD)
ifeq ($(wildcard $(BOARD_DIR)/.),)
   $(error Invalid BOARD specified)
endif

# If the board is different than the active one, remove the active BSP file.
# The mtb_init target needs to be run again to re-initialize the MTB libraries.
ifneq ($(BOARD),$(ACTIVE_BOARD))
   $(info Board changed from '$(ACTIVE_BOARD)' to '$(BOARD)'. Re-initializing ModusToolbox libraries.)
   $(shell rm -f $(MTB_ACTIVE_BSP_FILE))
endif

mtb_init: $(MTB_ACTIVE_BSP_FILE)

$(MTB_ACTIVE_BSP_FILE):
	$(info )
	$(MAKE) mtb_bsp_init
	$(info Creating active BSP file: $@)
	$(Q) echo $(BOARD) >> $@
	$(info Initialized ModusToolbox libs for board $(BOARD))

# Added as separate target to ensure it is only run when 
# the .mtb_active_bsp file does not exist
mtb_bsp_init: mtb_deinit mtb_add_bsp mtb_set_bsp mtb_get_libs

mtb_get_libs:
	$(info )
	$(info Retrieving ModusToolbox dependencies ...)
	$(Q) $(MAKE) -C $(MTB_LIBS_DIR) getlibs MTB_PROJECTS="proj_cm33_s proj_cm33_ns"

mtb_add_bsp:
	$(info )
	$(info Adding board $(BOARD) dependencies ...)
	$(Q) cd $(MTB_LIBS_DIR); library-manager-cli --project . --add-bsp-name $(BOARD) --add-bsp-version $(BOARD_VERSION)

mtb_set_bsp: 
	$(info )
	$(info Setting board $(BOARD) as active ...)
	$(Q) cd $(MTB_LIBS_DIR); library-manager-cli --project . --set-active-bsp APP_$(BOARD)

mtb_deinit: mtb_clean
	$(info )
	$(info Removing mtb_shared, bsps, libs dirs, and metafiles...)
	-$(Q) cd $(MTB_LIBS_DIR); rm -rf bsps
	-$(Q) cd $(MTB_LIBS_DIR); rm -rf ../mtb_shared
	-$(Q) cd $(MTB_LIBS_DIR); find . -type f -name assetlocks.json -exec rm -f {} +
	-$(Q) cd $(MTB_LIBS_DIR); find . -type f -name .mtbqueryapi -exec rm -f {} +
	-$(Q) cd $(MTB_LIBS_DIR); find . -type f -name .ninja_log -exec rm -f {} +
	-$(Q) cd $(MTB_LIBS_DIR); find . -type d -name libs -exec rm -rf {} +
	-$(Q) rm -f $(MTB_ACTIVE_BSP_FILE)

mtb_bsp_help:
	@:
	$(info )
	$(info ModusToolbox BSP setup available targets:)
	$(info )
	$(info 	mtb_init            Initialize ModusToolbox libraries for the selected board.)
	$(info  ..                  It depends on mtb_deinit, mtb_add_bsp, mtb_set_bsp and mtb_get_libs)
	$(info  mtb_add_bsp         Add the selected board BSP to the project)
	$(info  mtb_set_bsp         Set the selected board as active)
	$(info  mtb_get_libs        Download ModusToolbox libraries and dependencies)
	$(info	mtb_deinit          Remove ModusToolbox libraries and dependencies)
	$(info 	mtb_bsp_help        Show this help message)
	$(info )

.PHONY: mtb_deinit mtb_add_bsp mtb_set_bsp mtb_get_libs mtb_bsp_help