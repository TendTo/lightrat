load("@depend_on_what_you_use//dwyu/cc:defs.bzl", "dwyu_cc_aspect_factory")

# Providing no arguments means using the default configuration
dwyu = dwyu_cc_aspect_factory(skip_external_targets = True, analysis_optimizes_impl_deps = True, recursive = True)
