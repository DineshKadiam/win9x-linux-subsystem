%define arg(i) ebp + (8 + (i * 4))

; %define WSL9X_THREAD_TYPE 'WSL9'
%define WSL9X_THREAD_TYPE 0x394c5357
%define WSL9X_BLOCK_ID WSL9X_THREAD_TYPE
