--TEST--
enchant_dict_get_error() function
--CREDITS--
marcosptf - <marcosptf@yahoo.com.br>
--EXTENSIONS--
enchant
--SKIPIF--
<?php
if (!enchant_broker_list_dicts(enchant_broker_init())) {die("skip no dictionary installed on this machine! \n");}
?>
--FILE--
<?php
$broker = enchant_broker_init();
$dicts = enchant_broker_list_dicts($broker);

if (is_object($broker)) {
    echo("OK\n");
    $requestDict = enchant_broker_request_dict($broker, $dicts[0]['lang_tag']);

    if ($requestDict) {
        var_dump(enchant_dict_get_error($requestDict));
    } else {
        echo("broker request dict failed\n");
    }
} else {
    echo("init failed\n");
}
echo("OK\n");
?>
--EXPECT--
OK
bool(false)
OK
