# General

- Only update 3rd party dependencies which are not BoofCV infrequently
  - Typically every year or two
- Do not update CMake unless necessary
- Gradle wrapper uses the same version as BoofCV

# BoofCV

- Concurrency: Single and multithread code are both deterministic when given the same input/configuration and produce the same output
  - If this is discovered to not be the case it is most likely a bug, review BoofCV's code to see if it's intentional
  - Rule can be broken but needs an extremely good reason and must be documented

# BoofCPP

- Pure C++ implementation is designed as a standalone library that can be used by itself without Java BoofCV
  - Secondary goal and for now isn't fully fleshed out
- (Future) Two C++ libraries are provided; strict and fast
  - Strict reproduces exact same results of BoofCV, warts and all
  - Fast reproduces BoofCV up to floating point tolerance and can produce different results depending on architecture
  - Only one implementation right now and that is `strict`, remove (Future) and this comment when updated.
- C++ standard is specified in the root `CMakeLists.txt`
- Java version will match BoofCV
- Library version will match BoofCV's version
- Templates are only used when they greatly simplify the code, e.g. image data types
- Use virtual functions for polymorphism is preferred over templates except in hot loops or when it makes it more complex
- Keep C++ code simple and prefer safe code unless it's proven to be a performance hit
  - Don't blindly follow C++ performance rules of thumb, prove them with benchmarks
  - Humans: That expert who said you should always use complex C++ idioms because they are faster is prematurely optimizing, writes unmaintainable code to feel smart, and should be ignored
- Comments
  - C++ uses Doxygen style comments with ///
  - Java uses Markdown Javadoc
  - Comments describe the code as it is, not its history
- Configurations
  - C++ configurations have the same field names (in snake case) and default values as the Java configuration
  - They implement `visit_fields()`, which lists every field with its Java name
  - A test checks the field names and defaults against Java and that every field survives a copy to C++ and back
- JNI
  - C++ exceptions must never escape a JNI function. Catch them and throw a Java exception instead
  - No JNI calls while a critical array (`GetPrimitiveArrayCritical`) is held
  - If a configuration isn't supported natively, BoofCV must fall back to its Java implementation, e.g. the override returns `null`
