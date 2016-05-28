#!/usr/bin/perl

use strict;
use Pod::Html;

my %files = (
	       '../Net/Peep/Peep.pm' => 'peep-client.html',
	       '../Net/Peep/Conf/Conf.pm' => 'client/Net::Peep::Conf.html',
	       '../Net/Peep/Parse/Parse.pm' => 'client/Net::Peep::Parse.html',
	       '../Net/Peep/BC/BC.pm' => 'client/Net::Peep::BC.html',
	       '../Net/Peep/Log/Log.pm' => 'client/Net::Peep::Log.html',
	       '../Net/Peep/Client/Client.pm' => 'client/Net::Peep::Client.html',
	       '../Net/Peep/Client/Logparser/Logparser.pm' => 'client/Net::Peep::Client::Logparser.html',
	       '../Net/Peep/Client/Sysmonitor/Sysmonitor.pm' => 'client/Net::Peep::Client::Sysmonitor.html',
	       '../Net/Peep/Peck/Peck.pm' => 'client/Net::Peep::Peck.html',
	       '../bin/logparser' => 'client/logparser.html',
	       '../bin/sysmonitor' => 'client/sysmonitor.html',
	       '../bin/peck' => 'client/peck.html',
	       '../bin/keytest' => 'client/keytest.html'
	       );

my %regex = (
             '  Net::Peep::Conf      The Peep configuration object.  An  ' => 
             '  <A HREF="./client/Net::Peep::Conf.html">Net::Peep::Conf</A>           The Peep configuration object.  An  ',
             '  Net::Peep::Parse     The Peep configuration file parser.  Reads  ' =>
             '  <A HREF="./client/Net::Peep::Parse.html">Net::Peep::Parse</A>          The Peep configuration file parser.  Reads  ',
             '  Net::Peep::BC        The Peep broadcast module.  Used to notify Peep  ' =>
             '  <A HREF="./client/Net::Peep::BC.html">Net::Peep::BC</A>             The Peep broadcast module.  Used to notify Peep  ',
             '  Net::Peep::Log       Provides utility methods for logging, debugging,' =>
             '  <A HREF="./client/Net::Peep::Log.html">Net:::Peep::Log</A>            Provides utility methods for logging, debugging,',
             '  Net::Peep::Client    Base and utility class for Peep clients and ' => 
             '  <A HREF="./client/Net::Peep::Client.html">Net::Peep::Client</A>         Base and utility class for Peep clients and ',
             '  Net::Peep::Client::Logparser  Utility class for the logparser client.</PRE>' =>
             '  <A HREF="./client/Net::Peep::Client::Logparser.html">Net::Peep::Client::Logparser</A>  Utility class for the logparser client.</PRE>',
             '  Net::Peep::Client::Sysmonitor Utility class for the sysmonitor client.</PRE>' => 
             '  <A HREF="./client/Net::Peep::Client::Sysmonitor.html">Net::Peep::Client::Sysmonitor</A> Utility class for the sysmonitor client.</PRE>',
             '  Net::Peep::Peck      Utility class for the peck client.</PRE>' =>
             '  <A HREF="./client/Net::Peep::Peck.html">Net::Peep::Peck</A>           Utility class for the peck client.</PRE>',
             '  logparser            Generates events based on matching regular' =>
             '  <A HREF="./client/logparser.html">logparser</A>            Generates events based on matching regular',
             '  sysmonitor           Generates events based on system events (e.g.,' =>
             '  <A HREF="./client/sysmonitor.html">sysmonitor</A>           Generates events based on system events (e.g.,',
             '  peck                 Generates ad hoc Peep events.  Primarily for ' => 
             '  <A HREF="./client/peck.html">peck</A>                 Generates ad hoc Peep events.  Primarily for ',
             '  keytest              Generates ad hoc Peep events.  Primarily for ' => 
             '  <A HREF="./client/keytest.html">keytest</A>              Generates ad hoc Peep events.  Primarily for '
	     );

map { die "Error:  $_ not found" unless -e } sort keys %files;

print "\n";

for my $file (sort keys %files) {

    my $title = $files{$file};
    $title =~ s/\.html$//;
    $title =~ s|^client/||;
    $title =~ s/^peep-client$/Peep/;
    print "Generating HTML for $file ($title) ...\n";
    pod2html( "--infile=$file", "--outfile=$files{$file}", "--title=$title", "--header" );
    print "\tDone.\n";

}
    
print "Modifying peep-client.html ...\n";

open (F,'peep-client.html') || die "Cannot open peep-client.html:  $!";
my $html = join '', <F>;
close F;
for my $each (keys %regex) {
    my $regex = quotemeta($each);
    if ($html =~ /$regex/s) {
        print "\tFound regex [" . substr($each,0,40) ."]\n";
        print "\t\tSubstituting [" . substr($regex{$each},0,40) ."]\n";
        $html =~ s/$regex/$regex{$each}/s;
    } else {
        print "\tError:  Cannot find regex [" . substr($each,0,40) ."]!\n";
    }
}
open (F,'>peep-client.html') || die "Cannot open peep-client.html:  $!";
print F $html;
close F;

print "\tDone.\n";

print "Copying HTML files to the Peep docs directory ...\n";
print `cp -f peep-client.html ../../docs`;
print `cp -f client/*.html ../../docs/client`;
print "\tDone.\n";

print "\nDone.\n\n";

