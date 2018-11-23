#!/usr/bin/perl

use lib '.'; use lib 't';
use SATest; sa_t_init("spamd_symbols");
use Test; BEGIN { plan tests => 3 };

# ---------------------------------------------------------------------------

%patterns = (

q{ FROM_ENDS_IN_NUMS, }, 'endsinnums',
q{ NO_REAL_NAME, }, 'noreal',


);

ok (sdrun ("-L", "-y < data/spam/001", \&patterns_run_cb));
ok_all_patterns();

