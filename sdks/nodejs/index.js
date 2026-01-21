// Load the native addon
const decenlicense = require('./build/Release/decenlicense_node');

// Export the DecentriLicenseClient class
module.exports = decenlicense.DecentriLicenseClient;