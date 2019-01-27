# WARNING

BoofCPP is still in the early stages of development. Not all functions are optimized to be released for general 
distribution. A couple of functions can even some times be slower than the Java version. Various optimization
tricks are still being deployed, like getting the compiler to unroll inner loops under specific situations. It
doesn't do that by default because when it's built as a library it doesn't know the size of a kernel, as an example.

# Introduction

BoofCPP is a partial port the Java computer vision library [BoofCV](http://boofcv.org) to C++11.
It has been released under an Apache 2.0 open source license and is free to use and redistribute.
Ported operations are typically ones that are time consuming and likely to experience a significant
(at least 2x) speed improvement when compiled as native code. These are typically array heavy operations,
which can be difficult for the Java Virtual Machine (JVM) to fully optimise, e.g. SIMD and remove bounds
checking. Full compatibility (up to floating point precision) with the original library is a design goal
and enforced with unit tests that compare the results generated in BoofCV to BoofCPP.

It's very easy to add BoofCPP to your project. For most people this will involve adding a dependency to their
[Gradle](https://gradle.org/) or [Maven](https://maven.apache.org/) build script and
one line code code to their project. If you wish to build a custom version of BoofCPP
for your architecture please follow the build instructions below. Those who do not RTFM are likely to be punished with
code that is 2x to 3x slower than the pure Java code.

# Adding BoofCPP to Your Project

Pre-build code can be found on Maven Central (IT'S NOT THERE YET!!) and can be included as a dependency without 
building BoofCPP. Support is provided for the following architectures android-arm, linux-x86_64, macosx-x86_64,
and windows-x86_64. Examples for how to add depedencies in a Gradle script are provided below.

```Gradle
compile group: 'org.boofcpp', name: "boofcpp-jni", version: '0.30'
```

That just includes the Java library. For non-Android environments, you need to tell it which architecture to include
by specifying an architecture in the classifier field, e.g.

```Gradle
compile group: 'org.boofcpp', name: "boofcpp-jni", version: '0.30', classifier: "linux-x86_64"
```

For Android you don't need to specify the binary type but you do need to include the Android specific dependency:
```Gradle
compile group: 'org.boofcpp', name: "boofcpp-android", version: '0.30'
```

To use native libraries you need to do a very simple modification to your Java code. C++ code is injected into BoofCV 
through its BOverride API. Before invoking certain functions or creating
certain classes BoofCV will check to see if there exists a BOverride. If so it will invoke that instead.

```Java
BoofCPP.initialize()
```

Now all functions which BoofCPP supports will be overrided. This will also override any previously loaded overrides.
Initialize must be called before any image processing is done.

# Dependencies

The main source code has no external dependencies, making it very easy to build. However unit tests and benchmarks
have external dependencies. First make sure you have the gtest (unit tests) source code checked out:
```bash
cd boofcpp
git submodule update --init --recursive
```
Next install the following libraries required for benchmarking (Ubuntu 18.04):
```bash
sudo apt-get install libpng-dev libjpeg-dev
``` 

# Building

If you followed the dependency installation instructions above it should build without any issues.
To get the fastest code make sure you build it in release mode! By default it will
build in debug mode. Debug mode is great for development but many times slower than release code.

```bash
mkdir build;cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j8
```

Then to install the Java and Android libraries invoke the following command:
```bash
./gradlew install
```

## Android

```bash
cd boofcpp/boofcpp-android
../gradlew install
```

# Performance Benchmarks

The main benchmarks are done to directly compare Java and JNI code. There is also a second minimal set of C++ benchmarks
as a form of sanity check to make sure JNI or simply building the C++ code as a library does slow it down too much.

C++ Benchmark: boofcpp/benchmark
Java/JNI Benchmark: boofcpp/boofcpp-jni/src/benchmark

To run the C++ benchmark simply run the generated executable:
```bash
cd boofcpp/build/benchmark
./threshold
```

The Java benchmark code is created using JMH (https://openjdk.java.net/projects/code-tools/jmh/). The easiest
way to run it is to load the project in IntelliJ, install the JMH plugin, then run any of the benchmark class 
by right clicking it.

Example Results: Java/JNI Benchmark
```
Benchmark                                  Mode  Cnt    Score    Error  Units
BenchmarkThresholding.block_mean_java      avgt    5   18.321 ±  0.244  ms/op
BenchmarkThresholding.block_mean_native    avgt    5   12.769 ±  0.479  ms/op
BenchmarkThresholding.block_minmax_java    avgt    5   19.762 ±  0.352  ms/op
BenchmarkThresholding.block_minmax_native  avgt    5   21.616 ±  0.122  ms/op
BenchmarkThresholding.block_otsu_java      avgt    5   48.116 ±  1.703  ms/op
BenchmarkThresholding.block_otsu_native    avgt    5   35.232 ±  0.266  ms/op
BenchmarkThresholding.global_fixed_java    avgt    5    9.888 ±  0.812  ms/op
BenchmarkThresholding.global_fixed_native  avgt    5    1.260 ±  0.151  ms/op
BenchmarkThresholding.global_otsu_java     avgt    5   21.899 ±  1.578  ms/op
BenchmarkThresholding.global_otsu_native   avgt    5   13.223 ±  1.488  ms/op
BenchmarkThresholding.local_mean_java      avgt    5  100.411 ± 21.708  ms/op
BenchmarkThresholding.local_mean_native    avgt    5   71.637 ±  2.955  ms/op
```

Example Results: C++ Benchmark
```
        global_fixed time = 1.145900 (ms)
         global_otsu time = 12.192900 (ms)
          local_mean time = 64.670296 (ms)
       block_min_max time = 20.870100 (ms)
          block_mean time = 13.440100 (ms)
          block_otsu time = 37.993999 (ms)
```
NOTE: Most the performance difference between C++ and C++/JNI comes from being built as an application vs library.
When built as an application the compiler can unroll loops and other optimizations much easier.

# Trouble Shooting

* When I load this project into IntelliJ the IDE becomes unstable
** As of this writing, IntelliJ can't handle a project with the Java plugin and Android plugin
** Comment out the line which includes the Android plugin and restart IntelliJ

# Design Comments

BoofCPP has been designed and built to feel like a C++ library and not a Java library, but has Java roots.
In some cases limitations from Java are inherited, e.g. limits on array sizes and focus on certain data types.
Function and class names have been tweaked to follow C++ standards.

Changes from BoofCV
* Use of unsigned data types.
  * Justification: F*** Java's lack of unsigned data types and the hacks it forces upon people
* Array length's are unsigned 32-bit integerss (uint32_t)
  * Justification: Simplifies precondition checks, e.g. impossible to pass in a negative number
* Templates
  * Justification: Despite all of its flaws, C++ templates are easier to maintain than autogenerating code.
* Bloated Unit Tests:
  * Justification: Java reflections make iterating through functions and data types much easier and more concise

# Performance Comments

Java is much faster than most non-Java developers believe. Saying if C++
or Java is faster is a complex question. For well written Java and C++ code, operations which are array 
heavy and can be processed in sequence typically run 1.5x to 2.5x faster. Linear algebra is a perfect
example of code which can be optimized to run much faster in C++. For all other operations it's a
coin toss if Java or C++ is faster. If the code is not well written then typically C++ will run faster and 
crash harder than equally poorly written Java code.

BoofCPP has several good examples of the general rule stated in the previous paragraph. Very simple operations
run much faster in C++ (e.g. a simple global threshold) while complex algorithm with out of order operations
(e.g. contour finding) run at approximately the same speed in C++ and Java.

It's worth mentioning that the reason Java was used as the language for
BoofCV is because Java projects are easier to write and maintain as
they get larger, plus the development tools are better.

# Known Issues

For the most part everything should "just work", but if there are any known issues they will be listed here.

## IntelliJ

IntelliJ doesn't like projects which mix together the Java and Android plugin. It's recommended that you go into 
settings.gradle and comment out ```include 'boofcpp-android'```. If you don't do that it might appear to be work
but will have erratic behavior.