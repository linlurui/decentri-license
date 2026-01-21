use std::env;
use std::path::PathBuf;

fn main() {
    // Tell cargo to look for shared libraries in the specified directory
    println!("cargo:rustc-link-search=native=../../dl-core/build");
    
    // Tell cargo to tell rustc to link the decenlicense libraries
    println!("cargo:rustc-link-lib=dylib=decentrilicense");
    
    // Tell cargo to invalidate the built crate whenever the wrapper changes
    println!("cargo:rerun-if-changed=wrapper.h");
    println!("cargo:rerun-if-changed=../../sdks/cpp/include/decenlicense_c.h");

    // The bindgen::Builder is the main entry point
    // to bindgen, and lets you build up options for
    // the resulting bindings.
    let bindings = bindgen::Builder::default()
        // The input header we would like to generate bindings for.
        .header("../../sdks/cpp/include/decenlicense_c.h")
        // Restrict to our own API symbols to avoid flooding with libc types and lints.
        .allowlist_function("DL_.*")
        .allowlist_function("dl_.*")
        .allowlist_type("DL_.*")
        .allowlist_var("DL_.*")
        // Silence bindgen-generated naming/style lints for dependent libc types.
        .raw_line("#[allow(non_camel_case_types, non_upper_case_globals, non_snake_case)]")
        // Tell cargo to invalidate the built crate whenever any of the
        // included header files changed.
        .parse_callbacks(Box::new(bindgen::CargoCallbacks::new()))
        // Finish the builder and generate the bindings.
        .generate()
        // Unwrap the Result and panic on failure.
        .expect("Unable to generate bindings");
    
    // Write the bindings to the $OUT_DIR/bindings.rs file.
    let out_path = PathBuf::from(env::var("OUT_DIR").unwrap());
    bindings
        .write_to_file(out_path.join("bindings.rs"))
        .expect("Couldn't write bindings!");
}