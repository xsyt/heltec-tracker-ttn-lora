/*
 * TTN Payload Decoder
 * 
 * Use this JavaScript function in The Things Network Console
 * to decode the movement sensor data.
 * 
 * Copy this code to:
 * Application → Integrations → Payload Decoders → Decoder
 */

function decodeUplink(input) {
  var data = input.bytes;
  
  // Extract 4-byte big-endian unsigned integer
  var movements = (data[0] << 24) | (data[1] << 16) | (data[2] << 8) | data[3];
  
  return {
    data: {
      movements: movements,
      timestamp: new Date().toISOString()
    },
    warnings: [],
    errors: []
  };
}

/*
 * Alternative simpler version (for older TTN versions):
 * 
 * function Decoder(bytes, port) {
 *   return {
 *     movements: (bytes[0] << 24) | (bytes[1] << 16) | (bytes[2] << 8) | bytes[3]
 *   };
 * }
 */
