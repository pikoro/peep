#!/usr/bin/perl


BEGIN { $Test::Harness::verbose++; $|++; print "1..3\n"; }
END {print "not ok\n", exit 1 unless $loaded;}

use Net::Peep::Log;
use Net::Peep::Conf;
use Net::Peep::Parser;

$loaded = 1;

print "ok\n";

$Net::Peep::Log::debug = 0;

my ($conf,$parser);

eval {

	$conf = new Net::Peep::Conf;
	$parser = new Net::Peep::Parser;

};

if ($@) {
	print "not ok:  $@\n";
	exit 1;
} else {
	print "ok\n";
}

$Test::Harness::verbose += 1;
$Net::Peep::Log::debug = 1;

print STDERR "\nTesting Peep configuration file parser:\n";

eval { 

	$conf->setApp('logparser');
	$conf->setOption('config','./peep.conf');
	$parser->loadConfig($conf);

};

if ($@) {
	print STDERR "not ok:  $@";
	exit 1;
} else {
	print "ok\n";
	exit 0;
}

