--TEST--
readline_write_history(): Test that open_basedir is respected
--EXTENSIONS--
readline
--INI--
open_basedir=/tmp/some-sandbox
--FILE--
<?php

$name = '/tmp/out-of-sandbox';

var_dump(readline_write_history($name));

?>
--EXPECTF--
Warning: readline_write_history(): open_basedir restriction in effect. File(/tmp/out-of-sandbox) is not within the allowed path(s): (/tmp/some-sandbox) in %s on line %d
bool(false)
