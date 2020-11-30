# The function type checker is only supported on x86 and x86_64 for now.
if config.target_arch not in ['x86', 'x86_64', 'riscv64']:
  config.unsupported = True
