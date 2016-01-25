
define check-var-defined
  $(if $($(1)),,$(error "Variable $(1) undefined!"))
endef

$(call check-var-defined,CFG_CPUTYPE)
$(call check-var-defined,CFG_OSTYPE)
$(call check-var-defined,CFG_BUILD_DIR)