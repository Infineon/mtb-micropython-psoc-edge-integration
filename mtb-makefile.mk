
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
mtb_deinit: 
	$(info )
	$(info Removing mtb_shared and libs folder ...)
# 	-$(Q) cd $(MTB_LIBS_DIR); rm -rf bsps
# 	-$(Q) cd $(MTB_LIBS_DIR); rm -rf ../mtb_shared
# 	-$(Q) cd $(MTB_LIBS_DIR); rm -rf libs
# 	-$(Q) cd $(MTB_LIBS_DIR); find deps/*.mtb -maxdepth 1 -type f -delete 

mtb_build:
	$(info )
	$(info Building $(BOARD) in $(CONFIG) mode using MTB ...)
	$(Q) $(MAKE) -C $(MTB_LIBS_DIR) build 

.PHONY: mtb_init mtb_deinit mtb_build