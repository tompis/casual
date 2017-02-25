FROM centos:latest
MAINTAINER flurig <flurig@localhost>

RUN yum  -y install wget cmake make python rsync libuuid-devel sqlite-devel gcc-c++ zlib-devel unzip rpm-build
RUN wget http://dl.fedoraproject.org/pub/epel/7/x86_64/e/epel-release-7-9.noarch.rpm
RUN rpm -Uvh epel-release*rpm
RUN yum -y install pugixml yaml-cpp03

COPY casual-middleware-*.rpm /tmp/.
RUN rpm -i -p /tmp/casual-middleware-*.rpm
COPY start.sh /opt/casual/start.sh
RUN chmod 755 /opt/casual/start.sh

RUN useradd -ms /bin/bash casual

ENV CASUAL_HOME /opt/casual
ENV PATH $CASUAL_HOME/bin:$PATH
ENV LD_LIBRARY_PATH $LD_LIBRARY_PATH:$CASUAL_HOME/lib
ENV CASUAL_DOMAIN_HOME /test/casual
ENV CASUAL_LOG ".*"

RUN mkdir -p $CASUAL_DOMAIN_HOME
RUN cp -r /opt/casual/example/domain/single/minimal/* $CASUAL_DOMAIN_HOME/.
RUN cp $CASUAL_HOME/configuration/example/resources.yaml $CASUAL_HOME/configuration/.
RUN chown -R casual $CASUAL_DOMAIN_HOME
RUN chown -R casual $CASUAL_HOME/nginx

EXPOSE 8080 7771
USER casual
WORKDIR $CASUAL_DOMAIN_HOME

CMD ["/opt/casual/start.sh"]
