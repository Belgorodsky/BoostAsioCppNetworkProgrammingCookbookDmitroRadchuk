#!/bin/bash
openssl genrsa -out rootca.key 2048 && \
echo "" | awk '{  print "SU\nMoscow\nMoscow\nKremlin\nIT\Zero\ninfo@eblan.us\n\n" }' | openssl req -x509 -new -nodes -key rootca.key -days 20000 -out rootca.crt && \
openssl genrsa -out user.key 2048 && \
echo "" | awk '{  print "SU\nMoscow\nMoscow\nKremlin\nIT\One\ninfo@eblan.us\n\n" }' | openssl req -new -key user.key -out user.csr && \
openssl x509 -req -in user.csr -CA rootca.crt -CAkey rootca.key -CAcreateserial -out user.crt -days 20000 && \
openssl verify -CAfile rootca.crt rootca.crt && \
openssl verify -CAfile rootca.crt user.crt && \
openssl verify -CAfile user.crt user.crt || \
openssl dhparam -out dh2048.pem 2048
