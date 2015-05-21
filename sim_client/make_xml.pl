#!/usr/bin/perl
##---------------------------------------------------------------------------
#
# FILENAME	: make_xml.pl
# PROGRAM       : perl program
# CREATE DATE	: 2012/11/21
# MODIFY DATE	: 
# AUTHOR        : Ning Bin
# USAGE		: make_xml.pl -f TL1_LOG_FILE -o OUTPUT_FILE
# DESCRIPTION	: 
# RETURN CODE	: NONE
# 
# Copyright_2012_Ning Bin
##---------------------------------------------------------------------------
use POSIX qw(strftime);
use Getopt::Std;
use File::Basename;

$CUR_DIR=dirname($0);

# log file
$LOG_FILE="make_xml.log";

# usage
$USAGE="USAGE: $0 -F [TL1_LOG_FILE] -o [OUTPUT_FILE] \nEXAMPLE: $0 -c tl1-autodiscovery.log -o tl1.cfg\n\n";

# open log file
if( !open( LOG_FILE, ">>", "$LOG_FILE" ))
{
	print "Warning: can not write log $LOG_FILE : ($!)\nprocess continue...\n";
}
$info=&WriteLog("INFO");		# for writing info log
$error=&WriteLog("ERROR"); 		# for writing error log

# get the parameters
getopt("f:o:");

# check parameters
if ($opt_f)
{
    #if conf is specified
    $TL1_LOG_FILE=$opt_f;
}
else
{
	#if conf is not specified
	$error->("No parameter [TL1_LOG_FILE], quit");
	print "\nERROR: No parameter [TL1_LOG_FILE]. \n";
	die $USAGE;
}

if ($opt_o)
{
    #if output file is specified
    $OUTPUT_FILE=$opt_o;
}
else
{
	#if work output file is not specified
	$error->("No parameter [OUTPUT_FILE], quit");
	print "\nERROR: No parameter [OUTPUT_FILE]. \n";
	die $USAGE;
}

# start here
&Main;

# the Main function
sub Main
{
	$info->("$0 -f $TL1_LOG_FILE -o $OUTPUT_FILE begin");
	$info->("open files...");
	&OpenFile;
	$info->("make output file");
	&MakeOutput;
	$info->("$0 finish");
	close TL1_LOG_FILE;
	close OUTPUT_FILE;
	close LOG_FILE;
}

# log function
sub WriteLog
{
	my $logType = shift;
	$LogFunc = sub {
		my $event_time=strftime('%Y-%m-%d %H:%M:%S', localtime(time));		
		my $logContent = shift;
		print LOG_FILE "$event_time [$$] $logType: $logContent\n";		
	}
}

sub OpenFile
{
	# open conf file
	if( !open( $TL1_LOG_FILE, "<", "$TL1_LOG_FILE" ))
	{
		$error->("Can not read $TL1_LOG_FILE : ($!)");
		$info->("$0 end" );
		die "Can not read $TL1_LOG_FILE : ($!)\n";
	}
	
	# open output file
	if( !open( OUTPUT_FILE, ">", "$OUTPUT_FILE" ))
	{
		$error->("Can not write $OUTPUT_FILE : ($!)");
		$info->("$0 end" );
		die "Can not write $OUTPUT_FILE : ($!)\n";
	}
}

sub MakeOutput
{
      my $tid = "";
      my $curr_cmd = "";
      print OUTPUT_FILE "<?xml version=1.0?>\n\n";
      print OUTPUT_FILE "<CONFIGURATION>\n\n";
      while(<$TL1_LOG_FILE>)
      {
         chop;
         chomp;
	 			 next if( /^\s?$/ );		#null line
         next if( /^\r?$/ );    #null line

         if(/(.*):(.*):(.*):(.*);/)
         {
            ($tid,$curr_cmd) = &processTag();
         }
         elsif(/^;/)
         {
            &processTagClose($curr_cmd);
         }
         elsif(/^\s*$tid\s?\d\d-\d\d-\d\d/)
         {
            next;
         }
         elsif(/^M\s?.*/)
         {
            next;
         }
         elsif(/^>/)
         {
            next;
         }
         elsif(/^</)
         {
            next;
         }
         else
         {
            s/\r//g;
            print OUTPUT_FILE;
            print OUTPUT_FILE "\n";
         }
      }
      print OUTPUT_FILE "<CANC-USER>\n</CANC-USER>\n";
      print OUTPUT_FILE "\n</CONFIGURATION>\n";
}

sub processTag()
{
      my @value=split( /:/ );
      my $cmd = $value[0];
      my $tid = $value[1];
      my $aid = $value[2];
      my $ctag = $value[3];
      my $para = "";
      if( $#value > 5)
      {
        $para = $value[6];
        $para =~ s/;//g;
        $para =~ s/,/:/g;
        $para =~ s/=/:/g;
        $para =~ s/"/:/g;
        $para =~ s/\s//g;
        
#       print "para=$para\n";
      }
      if($para ne "")
      {
         $cmd .= ":";
         $cmd .= $aid;
         $cmd .= ":";
         $cmd .= $para;

#        print "cmd=$cmd\n";
      }

      print OUTPUT_FILE "<";
      print OUTPUT_FILE $cmd;
      print OUTPUT_FILE "><![CDATA[\n";

      return ($tid,$cmd);
}

sub processTagClose()
{
   my $cmd = shift;
   print OUTPUT_FILE "]]></";
   print OUTPUT_FILE $cmd;
   print OUTPUT_FILE ">\n\n";
}


