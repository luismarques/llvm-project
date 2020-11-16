add_lldb_library(lldbPluginABISysV_riscv PLUGIN
  ABISysV_riscv.cpp

  LINK_LIBS
    lldbCore
    lldbSymbol
    lldbTarget
  LINK_COMPONENTS
    Support
  )
