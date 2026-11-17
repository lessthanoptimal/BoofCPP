import org.gradle.api.Project
import org.gradle.api.publish.maven.MavenPom
import org.gradle.api.artifacts.dsl.RepositoryHandler

fun MavenPom.boofcppPom( pomName: String, pomDescription: String ) {
    name.set(pomName)
    description.set(pomDescription)
    url.set("https://boofcv.org")
    licenses {
        license {
            name.set("The Apache License, Version 2.0")
            url.set("http://www.apache.org/licenses/LICENSE-2.0.txt")
        }
    }
    developers {
        developer {
            id.set("pabeles")
            name.set("Peter Abeles")
            email.set("peter.abeles@gmail.com")
        }
    }
    scm {
        connection.set("scm:git:git://github.com/lessthanoptimal/BoofCPP.git")
        developerConnection.set("scm:git:git://github.com/lessthanoptimal/BoofCPP.git")
        url.set("https://github.com/lessthanoptimal/BoofCPP")
    }
}

/// Maven Central, the same repositories as BoofCV. Without credentials in ossrhUsername and ossrhPassword the build
/// still works, but publishing to them fails
fun RepositoryHandler.mavenCentralPublishing( project: Project ) {
    maven {
        val releasesRepoUrl = "https://ossrh-staging-api.central.sonatype.com/service/local/"
        val snapshotsRepoUrl = "https://central.sonatype.com/repository/maven-snapshots/"
        url = project.uri(if (project.version.toString().endsWith("SNAPSHOT")) snapshotsRepoUrl else releasesRepoUrl)
        credentials {
            username = project.findProperty("ossrhUsername") as String? ?: "dummy"
            password = project.findProperty("ossrhPassword") as String? ?: "dummy"
        }
    }
}

/// Publications are only signed when real credentials are present
fun Project.hasPublishingCredentials(): Boolean {
    val password = findProperty("ossrhPassword") as String?
    return password != null && password != "dummy"
}
