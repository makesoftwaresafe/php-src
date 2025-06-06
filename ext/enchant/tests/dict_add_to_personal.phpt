--TEST--
enchant_dict_add() function
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
$newWord = "iLoveJava";

if (is_object($broker)) {
    echo("OK\n");
    $requestDict = enchant_broker_request_dict($broker, $dicts[0]['lang_tag']);

    if ($requestDict) {
        echo("OK\n");
        $AddtoPersonalDict = enchant_dict_add($requestDict,$newWord);

        if (NULL === $AddtoPersonalDict) {
            var_dump($AddtoPersonalDict);

        } else {
            echo("dict add to personal failed\n");
        }

    } else {
    echo("broker request dict failed\n");

    }

} else {
    echo("broker is not a resource; failed;\n");
}
echo "OK\n";
?>
--EXPECT--
OK
OK
NULL
OK
