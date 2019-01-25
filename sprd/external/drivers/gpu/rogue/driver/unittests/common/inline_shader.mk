define rule-for-header-from-shader-txt
$(1)_type := binary_header
$(1)_src := $(1).txt
$(1)_target := $(1).txt.h
$(1)_name := $(1)
$(1)_is_string := true
endef
