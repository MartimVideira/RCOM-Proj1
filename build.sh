gcc -Wall -Wextra  src/frames.c src/link_layer.c test/read_noncanonical.c -o read  -lm
gcc -Wall -Wextra  src/frames.c src/link_layer.c test/write_noncanonical.c -o write -lm 
gcc -Wall -Wextra  src/application_layer.c src/frames.c src/link_layer.c   test/application.c -o app -lm
