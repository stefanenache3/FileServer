#!/bin/bash

for i in {1..15}
do
	echo "GET!9!file1.txt!" | nc -v 0.0.0.0 1000
done

for i in {1..5}
do
	echo "UPDATE!9!file1.txt!1!1!A!" | nc -v 0.0.0.0 1000
done


