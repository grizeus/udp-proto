make:
	gcc -o server ../server.c ../receive_from_client.c ../send_to_client.c ../extract_dns.c
	gcc -o client ../client.c
