
##############################################################
#
# AESDCHAR
#
##############################################################

#TODO: Fill up the contents below in order to reference your assignment 3 git contents
# Replace with your actual commit hash or version tag
AESDCHAR_VERSION = 'assignment8-complete'
# Note: Be sure to reference the *ssh* repository URL here (not https) to work properly
# with ssh keys and the automated build/test system.
# Your site should start with git@github.com:
# Example: AESDCHAR_SITE = 'git@github.com:cu-ecen-aeld/assignments-username.git'
AESDCHAR_SITE = '#TODO: ADD YOUR GIT REPO URL HERE'
AESDCHAR_SITE_METHOD = git
AESDCHAR_GIT_SUBMODULES = YES

# Kernel module build
AESDCHAR_MODULE_SUBDIRS = aesd-char-driver

define AESDCHAR_LINUX_CONFIG_FIXUPS
	$(call KCONFIG_ENABLE_OPT,CONFIG_DEBUG_FS)
endef

define AESDCHAR_INSTALL_TARGET_CMDS
	# Install the kernel module
	$(INSTALL) -D -m 0644 $(@D)/aesd-char-driver/aesdchar.ko $(TARGET_DIR)/lib/modules/$(LINUX_VERSION_PROBED)/extra/aesdchar.ko
	# Install load/unload scripts
	$(INSTALL) -m 0755 $(@D)/aesd-char-driver/aesdchar_load $(TARGET_DIR)/usr/bin/
	$(INSTALL) -m 0755 $(@D)/aesd-char-driver/aesdchar_unload $(TARGET_DIR)/usr/bin/
endef

$(eval $(kernel-module))
$(eval $(generic-package))
