
MTB_LIBS_DIR = $(TOP)/lib/mtb-psoc-edge-libs

mtb_init: mtb_deinit mtb_add_bsp mtb_set_bsp mtb_get_libs
	$(info )
	$(info Initializing ModusToolbox libs for board $(BOARD))

mtb_get_libs:
	$(info )
	$(info Retrieving ModusToolbox dependencies ...)
	$(Q) $(MAKE) -C $(MTB_LIBS_DIR) getlibs

mtb_add_bsp:
	$(info )
	$(info Adding board $(BOARD) dependencies ...)
	$(Q) cd $(MTB_LIBS_DIR); library-manager-cli --project . --add-bsp-name $(BOARD) --add-bsp-version $(BOARD_VERSION)

mtb_set_bsp: 
	$(info )
	$(info Setting board $(BOARD) as active ...)
	$(Q) cd $(MTB_LIBS_DIR); library-manager-cli --project . --set-active-bsp APP_$(BOARD)

# Remove MTB retrieved lib and dependencies
mtb_deinit: mtb_clean
	$(info )
	$(info Removing mtb_shared and libs folder ...)
	-$(Q) cd $(MTB_LIBS_DIR); rm -rf bsps
	-$(Q) cd $(MTB_LIBS_DIR); rm -rf ../mtb_shared
	-$(Q) cd $(MTB_LIBS_DIR); find . -type f -name assetlocks.json -exec rm -f {} +
	-$(Q) cd $(MTB_LIBS_DIR); find . -type f -name .mtbqueryapi -exec rm -f {} +
	-$(Q) cd $(MTB_LIBS_DIR); find . -type f -name .ninja_log -exec rm -f {} +
	-$(Q) cd $(MTB_LIBS_DIR); find . -type d -name libs -exec rm -rf {} +
#TODO: Check if this file is always created
	-$(Q) cd $(MTB_LIBS_DIR); rm -f proj_cm33_s/nsc_veneer.o 
# TODO: In future we might also need to remove the deps/*.mtb files as in PSOC6
# projects. I keep it here as a reminder.
# 	-$(Q) cd $(MTB_LIBS_DIR); find deps/*.mtb -maxdepth 1 -type f -delete 

mtb_clean:
	$(info )
	$(info Cleaning MTN build projects)
	-$(Q) $(MAKE) -C $(MTB_LIBS_DIR) clean
	-$(Q) cd $(MTB_LIBS_DIR); rm -rf build

# TODO: We don´t want to MTB init every time we build, only once. 
# For now, we will let the user explicitly call mtb_init.
# We will check later how to implement this effectively keeping the 
# required micropython flow:
#
# make submodules
# make BOARD=KIT_PSE84_AI
# make BOARD=KIT_PSE84_AI deploy
#
# Ideally, also we don´t need to specify the boards for every make command
# after the first one.

mtb_build:
	$(info )
	$(info Building $(BOARD) in $(CONFIG) mode using MTB ...)
	$(Q) $(MAKE) -C $(MTB_LIBS_DIR) build 

mtb_program:
	$(info )
	$(info Deploying firmware in board $(BOARD)...)
	$(info yes ${DEVICE_SN} is empty)
	$(Q) $(MAKE) -C $(MTB_LIBS_DIR) program MTB_PROBE_SERIAL=$(DEVICE_SN)

mtb_help:
	@:
	$(info )
	$(info ModusToolbox available targets:)
	$(info )
	$(info 	mtb_init            Initialize ModusToolbox libraries for the selected board.)
	$(info  ..                  It depends on mtb_deinit, mtb_add_bsp, mtb_set_bsp and mtb_get_libs)
	$(info  mtb_add_bsp         Add the selected board BSP to the project)
	$(info  mtb_set_bsp         Set the selected board as active)
	$(info  mtb_get_libs        Download ModusToolbox libraries and dependencies)
	$(info	mtb_deinit          Remove ModusToolbox libraries and dependencies)
	$(info 	mtb_build           Build the project using ModusToolbox build system)
	$(info 	mtb_program         Program the built firmware to the connected board.)
	$(info 	..                  Use DEVICE_SN to specify the board serial number)
	$(info 	mtb_clean           Clean the ModusToolbox build files)
	$(info 	mtb_help            Show this help message)
	$(info )

.PHONY: mtb_init mtb_deinit mtb_build