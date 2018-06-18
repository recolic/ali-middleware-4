
# Ali-middleware [![Build Status](https://travis-ci.com/recolic/ali-middleware.svg?token=e2iEcAqqTormZesZdC1C&branch=master)](https://travis-ci.com/recolic/ali-middleware)

> [Offical Document](https://code.aliyun.com/middlewarerace2018/docs)

# Curr
Failed to debug boost::coroutine and failed to finish unix-sock branch. Given up 10 hours before ddl.

dependency: github.com/recolic/grpc github.com/recolic/cpprestsdk github.com/recolic/etcd-cpp-apiv3 github.com/recolic/rlib

## note and TODO

```bash
# Clone tip:
git clone --recursive https://github.com/recolic/ali-middleware.git
# consumer-agent build(provider-agent and selector unfinished):
cd build && cmake -DCMAKE_BUILD_TYPE=Release .. && make -j4
# consumer-agent parallel testing
test/consumer.sh ./build
```
