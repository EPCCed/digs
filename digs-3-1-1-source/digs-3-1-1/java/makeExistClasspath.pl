#!/usr/bin/perl

open F, "existJars.txt";
@jars = <F>;
close( F );

foreach( @jars ){
  chomp( $_ );
  print( "$_:" );
}

