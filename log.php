<?php
  
$json = file_get_contents('php://input');
var_dump($json);

# file_put_contents( '/tmp/track01', date(), FILE_APPEND )
file_put_contents( '/tmp/track01', $json, FILE_APPEND );
file_put_contents( '/tmp/track01', "\n", FILE_APPEND );

$data = json_decode($json);
$export = var_export($data, true);
file_put_contents( '/tmp/track01', $export . "\n", FILE_APPEND );


# $array = get_object_vars($data);
# $properties = array_keys($array);
# file_put_contents( '/tmp/track01', var_export($properties, true) . "\n", FILE_APPEND);


if ( $data->{'reported_at'} ) {
    file_put_contents( '/tmp/track01', $data->{'reported_at'} . "\n" , FILE_APPEND );
}

# if ( $data->{'metadata'} ) {
#     file_put_contents( '/tmp/track01', var_export($data->{'metadata'}, true) . "\n" , FILE_APPEND );
# }

if ( $data->{'decoded'} ) {
    # file_put_contents( '/tmp/track01', $export['decoded'], FILE_APPEND );
    # file_put_contents( '/tmp/track01', "data->{'decoded'}\n" , FILE_APPEND );
    # file_put_contents( '/tmp/track01', var_export($data->{'decoded'}, true) . "\n" , FILE_APPEND );
    if ( $data->{'decoded'}->{'payload'} ) {
file_put_contents( '/tmp/track01', var_export($data->{'decoded'}->{'payload'}, true) . "\n" , FILE_APPEND );
    }
}

?>
Okay
