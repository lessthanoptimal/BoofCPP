import groovy.json.JsonOutput
import groovy.json.JsonSlurper
import org.gradle.api.DefaultTask
import org.gradle.api.file.DirectoryProperty
import org.gradle.api.file.RegularFileProperty
import org.gradle.api.tasks.InputDirectory
import org.gradle.api.tasks.OutputFile
import org.gradle.api.tasks.TaskAction
import java.io.File
import java.util.Locale
import kotlin.math.abs

/// Writes the markdown and HTML reports of a platform's saved benchmark results, see [BenchmarkReport]
abstract class BenchmarkReportTask : DefaultTask() {
    /// Directory with each implementation's results
    @get:InputDirectory
    abstract val results: DirectoryProperty

    @get:OutputFile
    abstract val report: RegularFileProperty

    @get:OutputFile
    abstract val html: RegularFileProperty

    init {
        // Always runs, so it always prints where the reports are
        outputs.upToDateWhen { false }
    }

    @TaskAction
    fun write() {
        val dir = results.get().asFile
        written(report.get().asFile, BenchmarkReport.render(dir))
        written(html.get().asFile, BenchmarkReport.renderHtml(dir))
    }

    /// Writes 'text' to 'file' and prints where
    protected fun written( file: java.io.File, text: String ) {
        file.writeText(text)
        logger.lifecycle("Wrote ${file.absolutePath}")
    }
}

/// Tables comparing BoofCV's Java code with each BoofCPP implementation. The markdown has a summary of Java's times and
/// each implementation's speedup, then each benchmark's times with their error. The HTML page has the summary, with
/// filters, sorting, and a switch between speedups and times. Each implementation has two files: `<impl>.json`, the
/// JMH results of a full run, and `<impl>.metadata.json`, which describes that run
object BenchmarkReport {
    /// Implementations in column order. Others follow, sorted by name
    private val ORDER = listOf("java", "strict", "fast")

    /// Image types in the order of the summary's columns. Others follow, sorted by name
    private val TYPES = listOf("U8", "S8", "U16", "S16", "S32", "F32", "F64")

    /// Speedups below this are in bold. Hooks must be at least this much faster than BoofCV's Java code
    const val MIN_SPEEDUP = 1.05

    /// One JMH result. 'params' doesn't have the implementation
    class Result(
        val benchmark: String, val op: String, val impl: String, val params: Map<String, String>,
        val score: Double, val error: Double, val unit: String
    )

    /// The saved results of each implementation, in column order, and their metadata, null if it's missing
    private class Saved( val impls: List<String>, val metadata: Map<String, Map<String, Any?>?>,
                         val results: List<Result> ) {
        /// True if the implementations ran on different CPUs or JDKs
        val mixed: Boolean
            get() {
                val known = metadata.values.filterNotNull()
                return known.map { it["cpu"] }.distinct().size > 1 || known.map { it["jdk"] }.distinct().size > 1
            }
    }

    private fun load( dir: File ): Saved {
        val impls = (dir.listFiles() ?: emptyArray())
            .map { it.name }
            .filter { it.endsWith(".json") && !it.endsWith(".metadata.json") }
            .map { it.removeSuffix(".json") }
            .sortedWith(compareBy({ ORDER.indexOf(it).let { i -> if (i < 0) ORDER.size else i } }, { it }))
        require(impls.isNotEmpty()) { "No results in $dir" }
        return Saved(impls, impls.associateWith { readMetadata(File(dir, "$it.metadata.json")) },
            impls.flatMap { readResults(File(dir, "$it.json"), it) })
    }

    fun render( dir: File ): String {
        val saved = load(dir)
        val out = StringBuilder("# Benchmarks: ${dir.name}\n\n")
        header(out, saved)
        summary(out, saved.results, saved.impls)
        out.append("## Details\n\n")
        for ((benchmark, rows) in saved.results.groupBy { it.benchmark }.toSortedMap())
            table(out, benchmark, rows, saved.impls)
        return out.toString()
    }

    /// The HTML page, the template in resources with the results as JSON
    fun renderHtml( dir: File ): String {
        val saved = load(dir)
        val labels = Labels(saved.results)
        val found = saved.results.mapNotNull { it.params["type"] }.distinct()
        val data = mapOf(
            "impls" to saved.impls,
            "types" to TYPES.filter { it in found } + found.filter { it !in TYPES }.sorted(),
            "minSpeedup" to MIN_SPEEDUP,
            "warning" to saved.mixed,
            "metadata" to saved.metadata.map { (impl, m) ->
                mapOf("impl" to impl, "date" to (m?.get("date")?.toString() ?: "metadata missing"),
                    "build" to (m?.let { build(it) } ?: ""), "cpu" to (m?.get("cpu")?.toString() ?: ""),
                    "jdk" to (m?.get("jdk")?.toString() ?: ""))
            },
            "results" to saved.results.map {
                mapOf("benchmark" to it.benchmark.substringAfterLast('.').removeSuffix("Benchmark"),
                    "label" to labels.of(it), "impl" to it.impl, "threads" to (it.params["threads"] ?: ""),
                    "type" to it.params["type"], "score" to it.score.takeIf { v -> !v.isNaN() },
                    "error" to it.error.takeIf { v -> !v.isNaN() }, "unit" to it.unit)
            })
        // "</" would end the script element
        val json = JsonOutput.toJson(data).replace("</", "<\\/")
        val template = BenchmarkReport::class.java.getResource("/benchmark_report.html")!!.readText()
        return template.replace("__TITLE__", "Benchmarks: ${dir.name}").replace("/*DATA*/null", json)
    }

    private fun build( m: Map<String, Any?> ): String =
        listOfNotNull(version("BoofCV", m["boofcv"]), version("BoofCPP", m["boofcpp"]), m["native"]?.toString())
            .joinToString(", ")

    private fun header( out: StringBuilder, saved: Saved ) {
        out.append("| Implementation | Date | Build | CPU | JDK |\n|---|---|---|---|---|\n")
        for ((impl, m) in saved.metadata) {
            if (m == null) {
                out.append("| $impl | metadata missing | | | |\n")
                continue
            }
            out.append("| $impl | ${m["date"] ?: ""} | ${build(m)} | ${m["cpu"] ?: ""} | ${m["jdk"] ?: ""} |\n")
        }
        out.append("\n")
        if (saved.mixed)
            out.append("**Warning:** the implementations ran on different CPUs or JDKs, so their times can't be " +
                    "compared.\n\n")
        out.append("Times are JMH averages with their 99.9% confidence error. Speedup is Java's time divided by the " +
                "implementation's; below ${MIN_SPEEDUP}x it's in bold.\n\n")
    }

    /// Java's time and each implementation's speedup over it: a column per image type for ops with an image type
    /// parameter, then a table of the other ops
    private fun summary( out: StringBuilder, results: List<Result>, impls: List<String> ) {
        out.append("## Summary\n\nJava's time, then each implementation's speedup over it.\n\n")
        val labels = Labels(results)
        val (typed, untyped) = results.partition { "type" in it.params }
        if (typed.isNotEmpty()) {
            val found = typed.map { it.params.getValue("type") }.distinct()
            val types = TYPES.filter { it in found } + found.filter { it !in TYPES }.sorted()
            summaryTable(out, typed, impls, labels, types) { it.params["type"] }
        }
        if (untyped.isNotEmpty())
            summaryTable(out, untyped, impls, labels, listOf("time")) { "time" }
    }

    /// The summary's name of an op: the benchmark's name without "Benchmark", the op, and the parameters which vary in
    /// its benchmark other than threads and image type, e.g. "ConvolveNormalized.horizontal radius=5"
    private class Labels( results: List<Result> ) {
        private val varying = results.groupBy { it.benchmark }.mapValues { (_, rows) ->
            rows.flatMap { it.params.keys }.distinct()
                .filter { it != "threads" && it != "type" }
                .filter { name -> rows.map { it.params[name] }.distinct().size > 1 }
                .sorted()
        }

        fun of( r: Result ): String {
            val name = r.benchmark.substringAfterLast('.').removeSuffix("Benchmark") + "." + r.op
            return (listOf(name) + varying.getValue(r.benchmark).map { "$it=${r.params[it]}" }).joinToString(" ")
        }
    }

    /// A row for each op, thread count and implementation, Java's first, with a cell for each column
    private fun summaryTable( out: StringBuilder, rows: List<Result>, impls: List<String>, labels: Labels,
                              columns: List<String>, column: (Result) -> String? ) {
        out.append((listOf("impl", "op", "threads") + columns).joinToString(" | ", "| ", " |\n"))
        out.append((listOf("impl", "op", "threads") + columns).joinToString("|", "|", "|\n") { "---" })
        val byKey = rows.groupBy { Triple(labels.of(it), it.params["threads"] ?: "", it.impl) }
        val keys = byKey.keys.map { it.first to it.second }.distinct()
            .sortedWith(compareBy<Pair<String, String>> { it.first }.thenBy { it.second.toIntOrNull() ?: 0 })
        for ((label, threads) in keys) {
            val java = byKey[Triple(label, threads, "java")] ?: emptyList()
            for (impl in impls) {
                val own = byKey[Triple(label, threads, impl)] ?: continue
                val cells = columns.map { name ->
                    val found = own.firstOrNull { column(it) == name } ?: return@map ""
                    val base = java.firstOrNull { column(it) == name }
                    if (impl == "java" || base == null) "${number(found.score)} ${found.unit.removeSuffix("/op")}"
                    else speedup(base, found)
                }
                out.append((listOf(impl, label, threads) + cells).joinToString(" | ", "| ", " |\n"))
            }
        }
        out.append("\n")
    }

    private fun version( name: String, value: Any? ): String? {
        val m = value as? Map<*, *> ?: return null
        val dirty = if (m["dirty"] == true) "(dirty)" else null
        return listOfNotNull(name, m["version"], m["sha"]?.toString()?.take(7), dirty).joinToString(" ")
    }

    private fun table( out: StringBuilder, benchmark: String, rows: List<Result>, impls: List<String> ) {
        val unit = rows.map { it.unit }.distinct().singleOrNull()
            ?: throw IllegalArgumentException("$benchmark has results in different units")
        val present = impls.filter { impl -> rows.any { it.impl == impl } }
        val faster = present.filter { it != "java" }
        val params = rows.flatMap { it.params.keys }.distinct().sorted()

        out.append("### ${benchmark.substringAfterLast('.')}\n\n")
        val columns = listOf("Op") + params + present.map { "$it ($unit)" } + faster.map { "$it speedup" }
        out.append(columns.joinToString(" | ", "| ", " |\n"))
        out.append(columns.joinToString("|", "|", "|\n") { "---" })

        val keys = rows.map { it.op to it.params }.distinct()
            .sortedWith(compareBy<Pair<String, Map<String, String>>> { it.first }
                .thenComparator { a, b -> compareParams(a.second, b.second, params) })
        for ((op, values) in keys) {
            val byImpl = rows.filter { it.op == op && it.params == values }.associateBy { it.impl }
            val java = byImpl["java"]
            val cells = listOf(op) + params.map { values[it] ?: "" } +
                    present.map { impl -> byImpl[impl]?.let { "${number(it.score)} ± ${error(it)}" } ?: "" } +
                    faster.map { impl -> speedup(java, byImpl[impl]) }
            out.append(cells.joinToString(" | ", "| ", " |\n"))
        }
        out.append("\n")
    }

    /// Numbers compare as numbers, e.g. threads 4 before 16
    private fun compareParams( a: Map<String, String>, b: Map<String, String>, names: List<String> ): Int {
        for (name in names) {
            val x = a[name] ?: ""
            val y = b[name] ?: ""
            val xn = x.toDoubleOrNull()
            val yn = y.toDoubleOrNull()
            val c = if (xn != null && yn != null) xn.compareTo(yn) else x.compareTo(y)
            if (c != 0)
                return c
        }
        return 0
    }

    private fun speedup( java: Result?, found: Result? ): String {
        if (java == null || found == null)
            return ""
        val ratio = java.score/found.score
        val text = String.format(Locale.ROOT, "%.2fx", ratio)
        return if (ratio < MIN_SPEEDUP) "**$text**" else text
    }

    /// Three significant digits, without an exponent
    fun number( value: Double ): String =
        if (value.isNaN()) "NaN" else String.format(Locale.ROOT, "%.${decimals(value)}f", value)

    /// The error with as many decimals as the score
    private fun error( result: Result ): String =
        if (result.error.isNaN()) "NaN" else String.format(Locale.ROOT, "%.${decimals(result.score)}f", result.error)

    private fun decimals( value: Double ): Int = when {
        value.isNaN() || abs(value) >= 100 -> 0
        abs(value) >= 10 -> 1
        abs(value) >= 1 -> 2
        else -> 3
    }

    private fun readMetadata( file: File ): Map<String, Any?>? {
        if (!file.exists())
            return null
        @Suppress("UNCHECKED_CAST")
        return JsonSlurper().parse(file) as Map<String, Any?>
    }

    private fun readResults( file: File, impl: String ): List<Result> {
        val entries = JsonSlurper().parse(file) as List<*>
        return entries.map { e ->
            val entry = e as Map<*, *>
            val name = entry["benchmark"].toString()
            val params = (entry["params"] as? Map<*, *> ?: emptyMap<Any, Any>())
                .map { it.key.toString() to it.value.toString() }.toMap()
            require(params["impl"] == impl) { "$file has a result of impl ${params["impl"]}" }
            val metric = entry["primaryMetric"] as Map<*, *>
            Result(name.substringBeforeLast('.'), name.substringAfterLast('.'), impl, params - "impl",
                toDouble(metric["score"]), toDouble(metric["scoreError"]), metric["scoreUnit"].toString())
        }
    }

    /// JMH writes NaN as a string
    private fun toDouble( value: Any? ): Double = (value as? Number)?.toDouble() ?: Double.NaN
}
