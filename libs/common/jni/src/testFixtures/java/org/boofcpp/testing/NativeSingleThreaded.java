package org.boofcpp.testing;

import org.boofcpp.NativeConcurrency;
import org.junit.jupiter.api.extension.AfterEachCallback;
import org.junit.jupiter.api.extension.BeforeEachCallback;
import org.junit.jupiter.api.extension.ExtensionContext;

/// Runs each test with one native thread and restores the thread count afterwards. For the tests of hooks with _MT
/// code, so the shared checks test the single threaded code. Use with `@ExtendWith(NativeSingleThreaded.class)`
public class NativeSingleThreaded implements BeforeEachCallback, AfterEachCallback {
    static final ExtensionContext.Namespace NAMESPACE = ExtensionContext.Namespace.create(NativeSingleThreaded.class);

    @Override public void beforeEach( ExtensionContext context ) {
        context.getStore(NAMESPACE).put("threads", NativeConcurrency.getMaxThreads());
        NativeConcurrency.setMaxThreads(1);
    }

    @Override public void afterEach( ExtensionContext context ) {
        NativeConcurrency.setMaxThreads(context.getStore(NAMESPACE).get("threads", Integer.class));
    }
}
