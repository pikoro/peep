use ExtUtils::MakeMaker;
# See lib/ExtUtils/MakeMaker.pm for details of how to influence
# the contents of the Makefile that is written.
WriteMakefile(
    'NAME'		=> 'Net::Peep',
    'VERSION'		=> '0.4.2-prerelease', # VERSION_FROM seems to break 5.00503
    'PREREQ_PM'		=> { 
      File::Tail => 0.98,
      Time::HiRes => 1.20,
      Net::Ping::External => 0.06 
      },
    'PM'                => {
                            'Peep.pm' => '$(INST_LIB)/Net/Peep.pm',
                            'bin/peck' => '$(INST_SCRIPT)/peck',
                            'bin/logparser' => '$(INST_SCRIPT)/logparser',
                            'bin/sysmonitor' => '$(INST_SCRIPT)/sysmonitor',
                            'bin/keytest' => '$(INST_SCRIPT)/keytest',
                           },
     ($] >= 5.005 ?
         ('ABSTRACT' => 'Client modules for Peep:  The Network Auralizer',
          'AUTHOR'   => 'Michael Gilfix <mgilfix@eecs.tufts.edu>') : ()),

);
