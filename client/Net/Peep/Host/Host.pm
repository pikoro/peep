package Net::Peep::Host;

require 5.005_62;
use strict;
use warnings;
use Carp;
use Socket;
require Exporter;

our @ISA = qw(Exporter);
our %EXPORT_TAGS = ( );
our @EXPORT_OK = ( );
our @EXPORT = qw( );
our $VERSION = do { my @r = (q$Revision: 1.1 $ =~ /\d+/g); sprintf "%d."."%02d" x $#r, @r };

use vars qw{ $AUTOLOAD @Attributes };

@Attributes = qw{ name ip description };

sub new {

	confess "This module is currently in development and not ready for prime time.  Sorry.  Try again later.";
	my $self = shift;
	my $class = ref($self) || $self;
	my $this = {};
	bless $this, $class;

} # end sub new

sub setName {

	my $self = shift;
	my $name = shift || confess "Name not found";
	$self->{'__NAME'} = $name;
	return 1;

} # end sub setName

sub getName {

	# if the host name has been explicitly set, retrieve the set value
	# ... if not obtain the value by gethostbyaddr

	my $self = shift;
	if (exists $self->{'__NAME'}) {
		return $self->{'__NAME'};
	} elsif (exists $self->{'__IP'}) {
		my $name = (gethostbyaddr($self->{'__IP'},AF_INET))[0];
		return $name ? $name : undef;
	} else {
		confess "Cannot obtain host name:  No host name or IP address has been specified.";
	}

} # end sub getIP

sub setIP {

	my $self = shift;
	my $ip = shift || confess "IP address not found";
	$self->{'__IP'} = $ip;
	return 1;

} # end sub setIP

sub getIP {

	# if the host IP address has been explicitly set, retrieve the set
	# value ... if not obtain the value by gethostbyname

	my $self = shift;
	if (exists $self->{'__IP'}) {
		return $self->{'__IP'};
	} elsif (exists $self->{'__NAME'}) {
		my $ip = inet_ntoa((gethostbyname($self->{'__NAME'}))[4]);
		return $ip ? $ip : undef;
	} else {
		confess "Cannot obtain IP address:  No host name or IP address has been specified.";
	}

} # end sub getIP

1;

__END__

=head1 NAME

Net::Peep::Host - The Peep host object

=head1 SYNOPSIS

  use Net::Peep::Host;

=head1 DESCRIPTION

The Peep host object.  Used to characterize a Peep host, including
attributes such as hostname and IP address.

=head2 EXPORT

=head1 AUTHOR

Collin Starkweather (C) 2001.

=head1 SEE ALSO

perl(1).

=cut
