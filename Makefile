comp:
	g++ src/*.cpp main.cpp -O2 -std=c++20 -lgmp -lmpfr -lpthread -fopenmp -frename-registers -funroll-loops -flto -D_GLIBCXX_PARALLEL -march=native -Wno-narrowing -Wl,--stack,8388608 -DNDEBUG 

run:
	clear && ./a.exe video out.mp4 --width=1920 --height=1080 --iters=75000 --real="-1.74934495027308084047378574996951414137319198025805813356741376505" --imag="0.00016914106112230739200115733184206598755687043390279361704845775" --zoom="1" --ezoom="8e37" --prec=300 --frames=20000 --framerate=120