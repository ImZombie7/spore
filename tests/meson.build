test_include = [
  include_directories('../kernel/src/include'),
  include_directories('../third_party/limine-protocol/include'),
]

test_pmm = executable('test_pmm',
  files(
    'test_pmm.c',
    '../kernel/src/mm/pmm.c',
    '../kernel/src/lib/mem.c',
  ),
  include_directories: test_include,
  native: true)

test('pmm', test_pmm)

