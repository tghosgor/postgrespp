{
  'variables': {
    'BuildDate': '<!(date --rfc-2822 --universal)',
  },
    
  'target_defaults': {
    'include_dirs': [ 'postgrespp', ],
    'cflags': [ '-std=c++14', '-march=x86-64', '-pthread', ],
    'ldflags': [ '-march=x86-64', '-pthread', ],
    'default_configuration': 'Debug',
    'configurations': {
      'Debug': {
        'cflags': [ '-g2', '-Wall' ],
        'ldflags': [ '-g2', ],
      },

      'Release': {
        'cflags': [ '-O3', ],
        'ldflags': [ '-O3', ],
      },
    },
  },  

  'targets': [
    {# test executable
      'target_name': 'test',
      'type': 'executable',
      'dependencies': [ 'postgrespp', ],
      'sources': [ '<!@(find testing -name *.cpp)', ],
    },

    {# library
      'target_name': 'postgrespp',
      'type': '<(library)',
      'link_settings': {
        'libraries': [ '-lpq', '-lboost_system', '-lboost_log', '-lboost_log_setup', ],
      },
      'sources': [ '<!@(find postgrespp/src -name *.cpp)', ],
      #'defines': [
      #  'BUILD_DATE=' '"<(BuildDate)"',],
    },
  ],
}
