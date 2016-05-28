package Net::Peep::Log;

require 5.005_62;
use strict;
use warnings;
use Time::HiRes qw{ gettimeofday tv_interval };

require Exporter;

our @ISA = qw(Exporter);
our %EXPORT_TAGS = ( 'all' => [ qw( ) ] );
our @EXPORT_OK = ( @{ $EXPORT_TAGS{'all'} } );
our @EXPORT = qw( );

use vars qw{ $debug $logfile $__LOGFILE $__LOGHANDLE };

our $VERSION = do { my @r = (q$Revision: 1.3 $ =~ /\d+/g); sprintf "%d."."%02d" x $#r, @r };

$debug = $__LOGFILE = 0;

$|++;

sub new {

    my $self = shift;
    my $class = ref($self) || $self;
    my $this = {};
    bless $this, $class

} # end sub new

sub log {

    my $self = shift;
    my $fh = $self->__logHandle();
    print $fh $self->__beautify(@_);

} # end sub log

sub debug {

    my $self = shift;
    my $level = shift;

    if ($debug >= $level) {

	if ($logfile) {
	    my $fh = $self->__logHandle();
	    print $fh $self->__beautify(@_);
	} else {
	    print STDERR $self->__beautify(@_);
	}

    }

} # end sub debug

sub __logHandle {

    my $self = shift;

    if (defined($__LOGHANDLE) ) {
	return $__LOGHANDLE;
    } elsif (defined $logfile) {
	print STDERR ref($self),":  Opening logfile $logfile ...\n";
	open(LOGFILE,">>$logfile") || die "Cannot open $logfile:  $!";
	$__LOGHANDLE = \*LOGFILE;
	$__LOGFILE++;
	return $__LOGHANDLE;
    } else {
	$__LOGHANDLE = \*STDOUT;
	return $__LOGHANDLE;
    }

} # end sub __logHandle

sub __beautify {

    my $self = shift;

    return "[" . scalar(localtime) . "] @_\n";

} # end sub __beautify

sub mark {

    # set a time against which future benchmarks can be measured
    my $self = shift;
    my $identifier = shift || die "Cannot set mark:  No identifier specified.";

    my $timeofday = gettimeofday;
    $self->{"__MARK"}->{$identifier} = [$timeofday];
    $self->debug(3,"Mark [$identifier:$timeofday] set.");

    return 1;

} # end sub mark

sub benchMark {

    # set a time against which future benchmarks can be measured
    my $self = shift;
    my $identifier = shift || die "Cannot acquire benchmark:  No identifier specified.";

    die "Cannot acquire benchmark:  No mark has been set with identifier '$identifier'."
	unless exists $self->{"__MARK"}->{$identifier};

    my $interval = tv_interval($self->{"__MARK"}->{$identifier},[gettimeofday]);

    $self->log("$interval seconds have passed since mark [$identifier:".$self->{"__MARK"}->{$identifier}."]");

    return $interval;

} # end sub mark

# one should endeavor to always exit gracefully

END { close LOGFILE if $__LOGFILE; }

1;

__END__

=head1 NAME

Net::Peep::Log - Perl extension for client-side logging and debugging for
Peep: The Network Auralizer.

This module is a part of Peep.

=head1 SYNOPSIS

  use Net::Peep::Log;
  my $log = new Net::Peep::Log;

  $log->log("Hello"," World!");

  $Net::Peep::Log::logfile = '/var/log/peep/client.log';
  $Net::Peep::Log::debug = 3;

  my $object = new Some::Object;
  $log->mark("foomark");
  $object->foo();
  $log->log("The method foo took ",$log->benchmark("foomark")," seconds.");
  $log->mark("barmark");
  $object->bar();
  $log->log("The method bar took ",$log->benchmark("barmark")," seconds and ",
            "foo and bar took a total of ",$log->benchmark("foomark"));

  $log->debug(1,"This message will be logged in ",
                "/var/log/peep/client.log because the debug level is 3");

  $log->debug(4,"This message will not be logged in ",
                "/var/log/peep/client.log because the debug level is 3");


=head1 DESCRIPTION

Net::Peep::Log provides methods for writing logging and debugging messages.

Messages are written to the file defined by the class attribute
C<$Net::Peep::Log::logfile>.  If the attribute is not defined, messages are
written to standard error.

All messages are prepended with a syslog-style time stamp.

Debugging messages are accompanied with an argument specifying the
debugging level of the message.  The class attribute C<$Net::Peep::Log::debug>
defines the cutoff level for debugging messages to appear in the log.

If the debugging level of a particular debugging message is compared
with the global debugging level.  If it is less than or equal to the
global debugging level, it is logged.  Otherwise, it is discarded.

The default value of C<$Net::Peep::Log::debug> is 0.

This provides the coder with the ability to specify how much debugging
output to include in client output by simply setting the
C<$Net::Peep::Log::debug> level.

Generally accepted categories associated with various debugging levels
are as follows:

  0 - No debugging output
  1 - Configuration debugging and output and command-line
      option parsing
  3 - General operational information
  5 - Log parsing and rule (e.g., event) recognition
  7 - Socket binding and client-server interaction
  9 - The works.  Whoa Nelly.  Watch out.

=head2 EXPORT

None by default.

=head2 CLASS ATTRIBUTES

  $Net::Peep::Log::VERSION - The CVS revision of this module.

The following class attributes are optional:

  $Net::Peep::Log::logfile - See below.
  will be written
  $Net::Peep::Log::debug - The debug level.  Default:  0.

If no logfile is specified, log output is sent to STDOUT and debugging
output is sent to STDERR.

If a logfile is specified, both log output and debugging output are
sent to the logfile.

=head2 PUBLIC METHODS

  new() - Net::Peep::Log constructor.

  log($message1,$message2,...) - Prints a log message.  All log
  messages are prepended with a syslog-style time stamp and appended
  with a newline.

  debug($debuglevel,$message1,$message2,...) - Prints a debugging
  message.  All debugging message are prepended with a syslog-style
  time stamp and appended with a newline.  Information regarding the
  debug level is given above.

  mark($identifier) - Starts marking time based on identifier
  $identifier.  Prints a log message to that effect.

  benchmark($identifier) - Evaluates the number of microseconds since
  the mark corresponding to identifier $identifier was set and prints
  a log message.  Returns the number of seconds accurate to the
  microsecond.

=head1 AUTHOR

Collin Starkweather <collin.starkweather@collinstarkweather.com>
Copyright (C) 2001

=head1 SEE ALSO

perl(1), peepd(1), Net::Peep::BC, Net::Peep::Parser, logparser,
sysmonitor.

http://peep.sourceforge.net

=head1 CHANGE LOG

$Log: Log.pm,v $
Revision 1.3  2001/07/23 17:46:29  starky
Added versioning to the configuration file as well as the ability to
specify groups in addition to / as a replacement for event letters.
Also changed the Net::Peep::Parse namespace to Net::Peep::Parser.
(I don't know why I ever named an object by a verb!)

Revision 1.2  2001/05/07 02:39:19  starky
A variety of bug fixes and enhancements:
o Fixed bug 421729:  Now the --output flag should work as expected and the
--logfile flag should not produce any unexpected behavior.
o Documentation has been updated and improved, though more improvements
and additions are pending.
o Removed print STDERRs I'd accidentally left in the last commit.
o Other miscellaneous and sundry bug fixes in anticipation of a 0.4.2
release.

Revision 1.1  2001/04/23 10:13:19  starky
Commit in preparation for release 0.4.1.

o Altered package namespace of Peep clients to Net::Peep
  at the suggestion of a CPAN administrator.
o Changed Peep::Client::Log to Net::Peep::Client::Logparser
  and Peep::Client::System to Net::Peep::Client::Sysmonitor
  for clarity.
o Made adjustments to documentation.
o Fixed miscellaneous bugs.

Revision 1.3  2001/03/31 07:51:35  mgilfix


  Last major commit before the 0.4.0 release. All of the newly rewritten
clients and libraries are now working and are nicely formatted. The server
installation has been changed a bit so now peep.conf is generated from
the template file during a configure - which brings us closer to having
a work-out-of-the-box system.

Revision 1.3  =head1 CHANGE LOG
 
$Log: Log.pm,v $
Revision 1.3  2001/07/23 17:46:29  starky
Added versioning to the configuration file as well as the ability to
specify groups in addition to / as a replacement for event letters.
Also changed the Net::Peep::Parse namespace to Net::Peep::Parser.
(I don't know why I ever named an object by a verb!)

Revision 1.2  2001/05/07 02:39:19  starky
A variety of bug fixes and enhancements:
o Fixed bug 421729:  Now the --output flag should work as expected and the
--logfile flag should not produce any unexpected behavior.
o Documentation has been updated and improved, though more improvements
and additions are pending.
o Removed print STDERRs I'd accidentally left in the last commit.
o Other miscellaneous and sundry bug fixes in anticipation of a 0.4.2
release.

Revision 1.1  2001/04/23 10:13:19  starky
Commit in preparation for release 0.4.1.

o Altered package namespace of Peep clients to Net::Peep
  at the suggestion of a CPAN administrator.
o Changed Peep::Client::Log to Net::Peep::Client::Logparser
  and Peep::Client::System to Net::Peep::Client::Sysmonitor
  for clarity.
o Made adjustments to documentation.
o Fixed miscellaneous bugs.

Revision 1.3  2001/03/31 07:51:35  mgilfix


  Last major commit before the 0.4.0 release. All of the newly rewritten
clients and libraries are now working and are nicely formatted. The server
installation has been changed a bit so now peep.conf is generated from
the template file during a configure - which brings us closer to having
a work-out-of-the-box system.

Revision 1.3  2001/03/31 02:17:00  mgilfix
Made the final adjustments to for the 0.4.0 release so everything
now works. Lots of changes here: autodiscovery works in every
situation now (client up, server starts & vice-versa), clients
now shutdown elegantly with a SIGTERM or SIGINT and remove their
pidfiles upon exit, broadcast and server definitions in the class
definitions is now parsed correctly, the client libraries now
parse the events so they can translate from names to internal
numbers. There's probably some other changes in there but many
were made :) Also reformatted all of the code, so it uses
consistent indentation.

Revision 1.2  2001/03/30 18:34:12  starky
Adjusted documentation and made some modifications to Peep::BC to
handle autodiscovery differently.  This is the last commit before the
0.4.0 release.

Revision 1.1  2001/03/28 00:33:59  starky
Adding the Peep::Log module for the first time.  I can't believe I forgot
to add this earlier.  Doh!


=cut

