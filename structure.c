//http://stackoverflow.com/questions/1577161/passing-a-structure-through-sockets-in-c
/*
Binary data should always be sent in a way that:

Handles different endianness
Handles different padding
Handles differences in the byte-sizes of intrinsic types
Don't ever write a whole struct in a binary way, not to a file, not to a socket.

Always write each field separately, and read them the same way.

You need to have functions like
*/
unsigned char * serialize_int(unsigned char *buffer, int value)
{
  /* Write big-endian int value into buffer; assumes 32-bit int and 8-bit char. */
  buffer[0] = value >> 24;
  buffer[1] = value >> 16;
  buffer[2] = value >> 8;
  buffer[3] = value;
  return buffer + 4;
}

unsigned char * serialize_char(unsigned char *buffer, char value)
{
  buffer[0] = value;
  return buffer + 1;
}

unsigned char * serialize_temp(unsigned char *buffer, struct temp *value)
{
  buffer = serialize_int(buffer, value->a);
  buffer = serialize_char(buffer, value->b);
  return buffer;
}

unsigned char * deserialize_int(unsigned char *buffer, int *value);

//Once you have the above, here's how you could serialize and transmit a structure instance:

int send_temp(int socket, const struct sockaddr *dest, socklen_t dlen,
              const struct temp *temp)
{
  unsigned char buffer[32], *ptr;

  ptr = serialize_temp(buffer, temp);
  return sendto(socket, buffer, ptr - buffer, 0, dest, dlen) == ptr - buffer;
}
/*
A few points to note about the above:

The struct to send is first serialized, field by field, into buffer.
The serialization routine returns a pointer to the next free byte in the buffer, which we use to compute how many bytes it serialized to
Obviously my example serialization routines don't protect against buffer overflow.
Return value is 1 if the sendto() call succeeded, else it will be 0.
*/
