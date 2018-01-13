#!groovy

//
// Use this to set version of casual
//

//
// Current version
//
env.CASUAL_VERSION="0.50"


def isPublishable()
{
   if ( "${env.BRANCH_NAME}" == "develop")
   {
      return true
   }
   return false
}

def common_backend_builder = '''
umask 000
cd /git/casual
find . -name "*.gcda" -print | xargs rm
find . -name "report.xml" -print | xargs rm
rm -rf middleware/.casual/unittest/.singleton/.domain-singleton
casual-make --no-colors -d -a --use-valgrind make && \
   casual-make --no-colors clean && \
   casual-make --no-colors -d -a --use-valgrind compile && \
   casual-make --no-colors -d -a --use-valgrind link && \
   ISOLATED_UNITTEST_DIRECTIVES="--gtest_output='xml:report.xml'" casual-make --no-colors test && \
   mkdir -p /git/casual/coverage && \
   casual-make --no-colors make && \
   casual-make --no-colors clean && \
   casual-make --no-colors compile && \
   casual-make --no-colors link && \
   casual-make --no-colors install && \
   cd /opt/casual && \
   tar cf /git/casual/casual-middleware.tar . && \
   cd /git/casual && python thirdparty/setup/install_nginx.py && \
   cd /opt/casual && \
   unzip -q /git/casual/casual-webapp.zip && \
   cd .. && \
   tar cf casual-middleware.tar casual && \
   cp casual-middleware.tar /git/casual/. && \
   cp /git/casual/package/casual-middleware.spec /root/rpmbuild/SPECS/. && \
   rpmbuild -bb --noclean --define "casual_version $CASUAL_VERSION" --define "casual_release $CASUAL_RELEASE" --define "distribution $DISTRIBUTION" /root/rpmbuild/SPECS/casual-middleware.spec  && \
   cp /root/rpmbuild/RPMS/x86_64/casual-middleware*.rpm /git/casual/. && \
   performCoverageMeasuring 
   returncode=$?

   cd /git/casual/
   find /git/casual -user root | xargs chmod 777
   f=$( ls casual-middleware-*.rpm )
   regexp="(casual-middleware-)(.*)(.x86_64.rpm)"
   if [[ $f =~ $regexp && $BRANCH != "develop" ]]
   then
      mv $f ${BASH_REMATCH[1]}${BASH_REMATCH[2]}-${BRANCH/\\//-}${BASH_REMATCH[3]}
   fi
   exit $returncode
'''

//
// Script for the backend builder centos
//
def backend_builder_centos = '''#! /bin/sh
CASUAL_VERSION=''' + env.CASUAL_VERSION + '''
CASUAL_RELEASE=''' + env.BUILD_NUMBER + '''
DISTRIBUTION="centos"
BRANCH=''' + env.BRANCH_NAME + '''

performCoverageMeasuring()
{
   cd /git/casual && \
   mkdir -p /git/casual/coverage && \
   gcovr -r . --html --html-details --gcov-exclude='.*thirdparty.*|.*test_.*' -o coverage/index.html
}

source scl_source enable devtoolset-4

''' + common_backend_builder

//
// Script for the backend builder suse
//
def backend_builder_suse = '''#! /bin/sh
CASUAL_VERSION=''' + env.CASUAL_VERSION + '''
CASUAL_RELEASE=''' + env.BUILD_NUMBER + '''
DISTRIBUTION="suse"
BRANCH=''' + env.BRANCH_NAME + '''

performCoverageMeasuring()
{
   echo "Perform no coverage measuring"
}


''' + common_backend_builder


//
// Script for the frontend builder
//
def frontend_builder = '''#! /bin/sh
umask 000
cd /git/casual/webapp
bower update --allow-root
touch bower_components/app-route/app-location.html
polymer build
cd ..
zip -q -r casual-webapp.zip webapp
chmod a+w -R .
'''

//
// Dockerfile for creating testcontainer
//
def dockerfile = '''FROM centos:latest
MAINTAINER flurig <flurig@localhost>

RUN yum  -y install wget cmake make python rsync libuuid-devel sqlite-devel gcc-c++ zlib-devel unzip rpm-build
RUN wget http://dl.fedoraproject.org/pub/epel/7/x86_64/Packages/e/epel-release-7-11.noarch.rpm
RUN rpm -Uvh epel-release*rpm
RUN yum -y install pugixml yaml-cpp03

ARG CASUAL_RPM
COPY ${CASUAL_RPM} /tmp/casual-middleware.rpm
RUN rpm -i -p /tmp/casual-middleware.rpm

RUN useradd -ms /bin/bash casual

ENV CASUAL_HOME /opt/casual
ENV PATH $CASUAL_HOME/bin:$PATH
ENV LD_LIBRARY_PATH $LD_LIBRARY_PATH:$CASUAL_HOME/lib
ENV CASUAL_DOMAIN_HOME /test/casual
ENV CASUAL_LOG '(error|warning|information)'

RUN mkdir -p $CASUAL_DOMAIN_HOME/logs
RUN cp -r /opt/casual/example/domain/single/minimal/* $CASUAL_DOMAIN_HOME/.
RUN cp $CASUAL_HOME/configuration/example/resources.yaml $CASUAL_HOME/configuration/.
RUN chown -R casual $CASUAL_DOMAIN_HOME
RUN chown -R casual $CASUAL_HOME/nginx
RUN ln -s /dev/stdout /test/casual/casual.log

EXPOSE 8080 7771
USER casual
WORKDIR $CASUAL_DOMAIN_HOME

ENTRYPOINT ["casual-domain-manager", "-c", "configuration/domain.yaml"]
'''

def build( name, image, content)
{
   def current_dir = pwd()
   writeFile file: 'builder.sh', text: content

   sh """
   chmod +x builder.sh
   docker pull $image
   docker run --rm -v $current_dir:/git/casual:Z $image /git/casual/builder.sh
   """
}

try
{
   slackSend color: "good", message: "Build started: ${env.JOB_NAME} - ${env.BUILD_NUMBER} (<${env.JOB_URL}|Open>)"

   parallel(
   
      "frontend":
      {
         node("frontend")
         {
            stage('Build frontend')
            {
               checkout scm
               
               build( 'casualfrontend', 'casual/frontend-builder', frontend_builder)

               stash includes: '**/casual-webapp.zip', name: 'frontend'
            }
         }
      },
      
      "documentation":
      {
         node( "documentation")
         {
            if ( isPublishable() )
            {
               stage('Documentation')
               {
                  checkout scm          
                  sh """
                    gitbook init .
                    gitbook build .
                  """
               }
            }
         }
      }
   )    
   
   parallel(
   
      "suse" :
      {
         node( "cplusplus" && "suse")
         {
            try
            { 

               deleteDir()

               stage('Build backend - Unittest - Suse')
               {
                  checkout scm
                  
                  unstash 'frontend'
                  
                  build( 'susecompile', 'casual/suse-builder', backend_builder_suse)

                  archive includes: '**/casual-middleware*.rpm'
                  
                  stash includes: '**/casual-middleware*.rpm', name: 'suse_backend'
               }
            }
            catch (Exception e)
            {
               archive includes: '**/casual.log'
               throw e
            }
         }
      },

      "centos" :
      {
         node( "cplusplus" && "centos")
         {
            try
            { 
               deleteDir()

               stage('Build backend - Unittest/CodeCoverage - Centos')
               {
                  checkout scm
                  
                  unstash 'frontend'
                  
                  build( 'centoscompile', 'casual/centos-builder', backend_builder_centos)

                  step([$class: 'XUnitBuilder',
                     thresholds: [[$class: 'FailedThreshold', failureThreshold: '1']],
                     tools: [[$class: 'GoogleTestType', pattern: '**/report.xml']]])

                  archive includes: '**/casual-middleware*.rpm'

                  stash includes: '**/casual-middleware*.rpm', name: 'centos_backend'

                  publishHTML (target: [
                     allowMissing: false,
                     alwaysLinkToLastBuild: false,
                     keepAll: true,
                     reportDir: 'coverage',
                     reportFiles: '*.html',
                     reportName: "GCOV Report"
                  ])
               }

               if ( isPublishable() )
               {
                  stage('Create container - Centos')
                  {
                     // 
                     // Setup files
                     //
                     writeFile file: 'Dockerfile', text: dockerfile

                     sh '''
                     docker build -t casual/middleware-centos --build-arg CASUAL_RPM=$(ls -t casual-middleware-*.rpm|head -1) -f Dockerfile .
                     '''
                  }

                  stage('Publishing to dockerhub - Centos')
                  {
                     sh """
                     docker tag casual/middleware-centos casual/middleware:latest
                     docker push casual/middleware
                     """
                  }

                  stage('Deploy - Centos')
                  {
                     sh """
                     if [[ -d /var/lib/jenkins/git/casual/docker ]]
                     then
                        cd /var/lib/jenkins/git/casual/docker
                        ./restart.sh
                     else
                        echo "Mock deploy"
                     fi
                     """
                  } 
               }
            }
            catch (Exception e)
            {
               archive includes: '**/casual.log'
               throw e
            }
         }
      }
   )

   node( "publishing")
   {
      if ( isPublishable() )
      {
         stage('publish')
         {
            unstash "suse_backend"
            unstash "centos_backend"

            withCredentials([usernameColonPassword(credentialsId: '628eddfb-34f4-46de-9e61-c73b86ac681e', variable: 'USERPASS')])
            {
               sh """
               set +x
               for filename in \$( ls casual-middleware*${BUILD_NUMBER}*.rpm )
               do
                  curl -X POST --user "${USERPASS}" "https://api.bitbucket.org/2.0/repositories/casualcore/casual/downloads" --form files=@"\$filename"
               done
               """
            }
         }

         stage('Tag')
         {
            checkout scm

            withCredentials([usernameColonPassword(credentialsId: '628eddfb-34f4-46de-9e61-c73b86ac681e', variable: 'USERPASS')])
            {
               tag = env.CASUAL_VERSION + "-" + env.BUILD_NUMBER
               sh """
               git tag -a -m "Jenkins" $tag
               git push https://${USERPASS}@bitbucket.org/casualcore/casual.git $tag
               """
            }
         }
      }
   }

   slackSend color: "good", message: "Build finished: ${env.JOB_NAME} - ${env.BUILD_NUMBER} (<${env.JOB_URL}|Open>)"
   currentBuild.result = 'SUCCESS'
}
catch (Exception e)
{
   slackSend color: "danger", message: "Build failed: ${env.JOB_NAME} - ${env.BUILD_NUMBER} (<${env.JOB_URL}|Open>)"
   currentBuild.result = 'FAILURE'
}

