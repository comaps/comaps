#pragma once

class Reader;
class Writer;

namespace rw_ops
{
/// Do reverse bytes.
/// Note! src and dest should be for different entities.
void Reverse(Reader const & src, Writer & dest);
}  // namespace rw_ops
