#!/usr/bin/perl

use lib '.'; use lib 't';
use SATest; sa_t_init("stripmarkup");
use Test; BEGIN { plan tests => 4 };

# ---------------------------------------------------------------------------

%patterns = (

q{ Content-Type: text/html }, 'contenttype',
q{ 
  Sender: pertand@email.mondolink.com

  <HTML></P>
  }, 'startofbody',

  q{Subject: "100% HERBALSENSATION"}, 'subj',

);

ok (sarun ("-d < data/spam/003", \&patterns_run_cb));
ok_all_patterns();
