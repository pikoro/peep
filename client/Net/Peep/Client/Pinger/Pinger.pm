package Net::Peep::Client::Pinger;

require 5.005_62;
use strict;
use warnings;
use Carp;
use Data::Dumper;
use File::Tail;
use Net::Peep::Client;
use Net::Peep::BC;
use Net::Ping::External qw(ping);

require Exporter;

our @ISA = qw(Exporter Net::Peep::Client);
our %EXPORT_TAGS = ( 'all' => [ qw( INTERVAL MAX_INTERVAL ADJUST_AFTER ) ] );
our @EXPORT_OK = ( @{ $EXPORT_TAGS{'all'} } );
our @EXPORT = qw( );
our $VERSION = do { my @r = (q$Revision: 1.1 $ =~ /\d+/g); sprintf "%d."."%02d" x $#r, @r };

use constant DEFAULT_PID_FILE => "/var/run/pinger.pid";
use constant DEFAULT_INTERVAL => 60; # ping every 60 seconds

sub new {

	confess "This module is still in development and is not ready for prime time.  Sorry.  Please try again later.";
	my $self = shift;
	my $class = ref($self) || $self;
	my $this = $class->SUPER::new('pinger');
	bless $this, $class;

} # end sub new

sub Start {

	my $self = shift;

	my $interval = DEFAULT_INTERVAL;
	my $pidfile = DEFAULT_PID_FILE;

	my %options = ( 'interval=s' => \$interval, 'pidfile=s' => \$pidfile );

	$self->parseopts(%options) || $self->pods();

	$self->parseconf();

	my $conf = $self->getconf();

	unless ($conf->getOption('autodiscovery')) {
		$self->pods("Error:  Without autodiscovery you must provide a server and port option.")
			unless $conf->optionExists('server') && $conf->optionExists('port') &&
			       $conf->getOption('server') && $conf->getOption('port');
	}

	# Check whether the pidfile option was set. If not, use the default
	unless ($conf->optionExists('pidfile') && $conf->getOption('pidfile')) {
		$self->logger()->debug(3,"No pid file specified. Using default [" . DEFAULT_PID_FILE . "]");
		$conf->setOption('pidfile', DEFAULT_PID_FILE);
	}

	# Check whether the interval option was set. If not, use the default
	unless ($conf->optionExists('interval') && $conf->getOption('interval')) {
		$self->logger()->debug(3,"No interval specified. Using default [" . DEFAULT_INTERVAL . "]");
		$conf->setOption('pidfile', DEFAULT_INTERVAL);
	}

	$self->logger()->debug(9,"Registering callback ...");

	$self->callback(sub { $self->loop($interval); });

	$self->logger()->debug(9,"\tCallback registered ...");

	$self->MainLoop();

	return 1;

} # end sub Start

sub loop {

    my $self = shift;

    my $conf = $self->getconf();

    my @hosts = $self->getClientHosts('pinger');

    for my $host (@hosts) {

	my $name = $host->getName();
	my $ip = $host->getIP() || gethostbyname($name);
	$self->logger()->debug(9,"Pinging host [$name] with IP [$ip] ...");
	my $alive = ping( host => $ip );
	if ($alive) {
	    $self->logger()->debug(9,"\tHost [$name] alive.");
	} else {
    	    $self->logger()->debug(9,"\tHost [$name] unresponsive!");
	}

    }

} # end sub loop

sub peck {

	my $self = shift;

	my $configuration = $self->getconf();

	unless (exists $self->{"__PEEP"}) {
		if ($configuration->getOptions()) {
			$self->{"__PEEP"} = Net::Peep::BC->new( 'pinger', $configuration );
		} else {
			confess "Error:  Expecting options to have been parsed by now.";
		}
	}

	return $self->{"__PEEP"};

} # end sub peck

1;

__END__
# Below is stub documentation for your module. You better edit it!

=head1 NAME

Net::Peep::Client::Pinger - Perl extension for checking whether a host
or a variety of services on a host are running

=head1 SYNOPSIS

  use Net::Peep::Client::Pinger;
  blah blah blah

=head1 DESCRIPTION

Stub documentation for Net::Peep::Client::Pinger, created by h2xs. It looks like the
author of the extension was negligent enough to leave the stub
unedited.

Blah blah blah.

=head2 EXPORT

None by default.


=head1 AUTHOR

A. U. Thor, a.u.thor@a.galaxy.far.far.away

=head1 SEE ALSO

perl(1).

=cut
