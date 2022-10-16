gcc -Wall -Wextra  src/frames.c src/link_layer.c test/read_noncanonical.c -o read
gcc -Wall -Wextra  src/frames.c src/link_layer.c test/write_noncanonical.c -o write
gcc -Wall -Wextra  src/application_layer.c   test/application.c -o app
