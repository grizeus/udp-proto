make:
	gcc -o server ../server.c ../receive_from_client.c ../send_to_client.c
	gcc -o client ../client.c
