// swift-tools-version:5.9
import PackageDescription

let package = Package(
    name: "DecentriLicense",
    products: [
        .library(name: "DecentriLicense", targets: ["DecentriLicense"]),
    ],
    targets: [
        .systemLibrary(
            name: "CDecentriLicense",
            path: "Sources/CDecentriLicense",
            pkgConfig: "decentrilicense"
        ),
        .target(
            name: "DecentriLicense",
            dependencies: ["CDecentriLicense"],
            path: "Sources/DecentriLicense"
        ),
        .executableTarget(
            name: "ValidationWizard",
            dependencies: ["DecentriLicense"],
            path: "Sources/ValidationWizard"
        ),
    ]
)
