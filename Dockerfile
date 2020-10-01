FROM miketruman/compiler AS builder

COPY main.cpp ./
COPY makefile ./
RUN make

FROM scratch
COPY --from=builder /build/a.bin /build/a.bin
EXPOSE 8080
CMD ["/build/a.bin" ]
