FROM scratch
COPY rootfs/ /
RUN ["/bin/mkdir", "-m", "1777", "-p", "/tmp", "/var/tmp"]
WORKDIR /work
ENV PATH=/usr/local/bin:/usr/bin:/bin:/usr/sbin:/sbin
CMD ["/bin/sleep", "2147483647"]
