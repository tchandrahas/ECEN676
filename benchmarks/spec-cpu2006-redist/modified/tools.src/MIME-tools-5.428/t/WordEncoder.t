use strict;
use warnings;
use Test::More tests => 8;
use MIME::Words qw(:all);

is(encode_mimeword('wookie', 'Q', 'ISO-8859-1'),
    '=?ISO-8859-1?Q?wookie?=');
is(encode_mimeword('Fran�ois', 'Q', 'ISO-8859-1'),
    '=?ISO-8859-1?Q?Fran=E7ois?=');
is(encode_mimewords('Me and Fran�ois'), '=?ISO-8859-1?Q?Me=20and=20Fran=E7ois?=');
is(decode_mimewords('=?ISO-8859-1?Q?Me=20and=20Fran=E7ois?='),
   'Me and Fran�ois');

is(encode_mimewords('Me and Fran�ois and Fran�ois    and Fran�ois       and Fran�ois               and Fran�ois                      and Fran�ois'),
   '=?ISO-8859-1?Q?Me=20and=20Fran=E7ois=20an?= =?ISO-8859-1?Q?d=20Fran=E7ois=20=20=20=20and=20?= =?ISO-8859-1?Q?Fran=E7ois=20=20=20=20=20=20=20and?= =?ISO-8859-1?Q?=20Fran=E7ois=20=20=20=20=20=20=20=20=20?= =?ISO-8859-1?Q?=20=20=20=20=20=20and=20Fran=E7ois?= =?ISO-8859-1?Q?=20=20=20=20=20=20=20=20=20=20=20=20=20=20=20=20=20=20?= =?ISO-8859-1?Q?=20=20=20=20and=20Fran=E7ois?=');


is(decode_mimewords('=?ISO-8859-1?Q?Me=20and=20Fran=E7ois=20an?= =?ISO-8859-1?Q?d=20Fran=E7ois=20=20=20=20and=20?= =?ISO-8859-1?Q?Fran=E7ois=20=20=20=20=20=20=20and?= =?ISO-8859-1?Q?=20Fran=E7ois=20=20=20=20=20=20=20=20=20?= =?ISO-8859-1?Q?=20=20=20=20=20=20and=20Fran=E7ois?= =?ISO-8859-1?Q?=20=20=20=20=20=20=20=20=20=20=20=20=20=20=20=20=20=20?= =?ISO-8859-1?Q?=20=20=20=20and=20Fran=E7ois?='),
   'Me and Fran�ois and Fran�ois    and Fran�ois       and Fran�ois               and Fran�ois                      and Fran�ois');


is(encode_mimewords('Me and Fran�ois and Fran�ois    and Fran�ois       and Fran�ois               and Fran�ois                      and Fran�ois and wookie and wookie and wookie and wookie and wookie and wookie'),
   '=?ISO-8859-1?Q?Me=20and=20Fran=E7ois=20an?= =?ISO-8859-1?Q?d=20Fran=E7ois=20=20=20=20and=20?= =?ISO-8859-1?Q?Fran=E7ois=20=20=20=20=20=20=20and?= =?ISO-8859-1?Q?=20Fran=E7ois=20=20=20=20=20=20=20=20=20?= =?ISO-8859-1?Q?=20=20=20=20=20=20and=20Fran=E7ois?= =?ISO-8859-1?Q?=20=20=20=20=20=20=20=20=20=20=20=20=20=20=20=20=20=20?= =?ISO-8859-1?Q?=20=20=20=20and=20Fran=E7ois=20a?=nd wookie and wookie and wookie and wookie and wookie and wookie');

is(decode_mimewords('=?ISO-8859-1?Q?Me=20and=20Fran=E7ois=20an?= =?ISO-8859-1?Q?d=20Fran=E7ois=20=20=20=20and=20?= =?ISO-8859-1?Q?Fran=E7ois=20=20=20=20=20=20=20and?= =?ISO-8859-1?Q?=20Fran=E7ois=20=20=20=20=20=20=20=20=20?= =?ISO-8859-1?Q?=20=20=20=20=20=20and=20Fran=E7ois?= =?ISO-8859-1?Q?=20=20=20=20=20=20=20=20=20=20=20=20=20=20=20=20=20=20?= =?ISO-8859-1?Q?=20=20=20=20and=20Fran=E7ois=20a?=nd wookie and wookie and wookie and wookie and wookie and wookie'),
   'Me and Fran�ois and Fran�ois    and Fran�ois       and Fran�ois               and Fran�ois                      and Fran�ois and wookie and wookie and wookie and wookie and wookie and wookie');

