
##############################################################
#
# AESD-ASSIGNMENTS
#
##############################################################

#TODO: Fill up the contents below in order to reference your assignment 3 git contents
# Replace with your actual commit hash or version tag
AESD_ASSIGNMENTS_VERSION = 'assignment8-complete'
# Note: Be sure to reference the *ssh* repository URL here (not https) to work properly
# with ssh keys and the automated build/test system.
# Your site should start with git@github.com:
# Example: AESD_ASSIGNMENTS_SITE = 'git@github.com:cu-ecen-aeld/assignments-username.git'
AESD_ASSIGNMENTS_SITE = '#TODO: ADD YOUR GIT REPO URL HERE'
AESD_ASSIGNMENTS_SITE_METHOD = git
AESD_ASSIGNMENTS_GIT_SUBMODULES = YES

define AESD_ASSIGNMENTS_BUILD_CMDS
	$(MAKE) $(TARGET_CONFIGURE_OPTS) -C $(@D)/finder-app all
	# Build aesdsocket from server directory if it exists
	if [ -d $(@D)/server ]; then \
		$(MAKE) $(TARGET_CONFIGURE_OPTS) -C $(@D)/server all; \
	fi
endef

# TODO add your writer, finder and finder-test utilities/scripts to the installation steps below
define AESD_ASSIGNMENTS_INSTALL_TARGET_CMDS
	$(INSTALL) -d 0755 $(@D)/conf/ $(TARGET_DIR)/etc/finder-app/conf/
	$(INSTALL) -m 0755 $(@D)/conf/* $(TARGET_DIR)/etc/finder-app/conf/
	$(INSTALL) -m 0755 $(@D)/assignment-autotest/test/assignment4/* $(TARGET_DIR)/bin
	# Install finder-app utilities
	$(INSTALL) -m 0755 $(@D)/finder-app/writer $(TARGET_DIR)/usr/bin/ 2>/dev/null || true
	$(INSTALL) -m 0755 $(@D)/finder-app/finder.sh $(TARGET_DIR)/usr/bin/ 2>/dev/null || true
	$(INSTALL) -m 0755 $(@D)/finder-app/finder-test.sh $(TARGET_DIR)/usr/bin/ 2>/dev/null || true
	# Install aesdsocket if built
	$(INSTALL) -m 0755 $(@D)/server/aesdsocket $(TARGET_DIR)/usr/bin/ 2>/dev/null || true
endef

$(eval $(generic-package))
