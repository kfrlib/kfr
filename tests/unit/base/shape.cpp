/**
 * KFR (https://www.kfrlib.com)
 * Copyright (C) 2016-2023 Dan Cazarin
 * See LICENSE.txt for details
 */

#include <kfr/base/shape.hpp>

namespace kfr
{
inline namespace CMT_ARCH_NAME
{

TEST(shape)
{
    using internal_generic::increment_indices_return;
    using internal_generic::null_index;
    CHECK(size_of_shape(shape{ 4, 3 }) == 12);
    CHECK(size_of_shape(shape{ 1 }) == 1);
    CHECK(size_of_shape<1>(1) == 1);
    shape<1> sh1 = 1;
    sh1          = 2;

    CHECK(internal_generic::strides_for_shape(shape{ 2, 3, 4 }) == shape{ 12, 4, 1 });

    CHECK(internal_generic::strides_for_shape(shape{ 2, 3, 4 }, 10) == shape{ 120, 40, 10 });

    CHECK(increment_indices_return(shape{ 0, 0, 0 }, shape{ 0, 0, 0 }, shape{ 2, 3, 4 }) == shape{ 0, 0, 1 });
    CHECK(increment_indices_return(shape{ 0, 0, 3 }, shape{ 0, 0, 0 }, shape{ 2, 3, 4 }) == shape{ 0, 1, 0 });
    CHECK(increment_indices_return(shape{ 0, 2, 0 }, shape{ 0, 0, 0 }, shape{ 2, 3, 4 }) == shape{ 0, 2, 1 });
    CHECK(increment_indices_return(shape{ 0, 2, 3 }, shape{ 0, 0, 0 }, shape{ 2, 3, 4 }) == shape{ 1, 0, 0 });
    CHECK(increment_indices_return(shape{ 1, 2, 3 }, shape{ 0, 0, 0 }, shape{ 2, 3, 4 }) ==
          shape{ null_index, null_index, null_index });

    CHECK(shape{ 3, 4, 5 }.to_flat(shape{ 0, 0, 0 }) == 0);
    CHECK(shape{ 3, 4, 5 }.to_flat(shape{ 2, 3, 4 }) == 59);

    CHECK(shape{ 3, 4, 5 }.from_flat(0) == shape{ 0, 0, 0 });
    CHECK(shape{ 3, 4, 5 }.from_flat(59) == shape{ 2, 3, 4 });
}
TEST(shape_broadcast)
{
    using internal_generic::can_assign_from;
    using internal_generic::common_shape;
    using internal_generic::same_layout;

    CHECK(common_shape(shape{ 1, 5 }, shape{ 5, 1 }) == shape{ 5, 5 });
    CHECK(common_shape(shape{ 5 }, shape{ 5, 1 }) == shape{ 5, 5 });
    CHECK(common_shape(shape{ 1, 1, 1 }, shape{ 2, 5, 1 }) == shape{ 2, 5, 1 });
    CHECK(common_shape(shape{ 1 }, shape{ 2, 5, 7 }) == shape{ 2, 5, 7 });

    CHECK(common_shape(shape{}, shape{ 0 }) == shape{ 0 });
    CHECK(common_shape(shape{}, shape{ 0, 0 }) == shape{ 0, 0 });
    CHECK(common_shape(shape{ 0 }, shape{ 0, 0 }) == shape{ 0, 0 });

    CHECK(common_shape<true>(shape{}, shape{ 0 }) == shape{ 0 });
    CHECK(common_shape<true>(shape{}, shape{ 0, 0 }) == shape{ 0, 0 });
    CHECK(common_shape<true>(shape{ 0 }, shape{ 0, 0 }) == shape{ 0, 0 });

    CHECK(can_assign_from(shape{ 1, 4 }, shape{ 1, 4 }));
    CHECK(!can_assign_from(shape{ 1, 4 }, shape{ 4, 1 }));
    CHECK(can_assign_from(shape{ 1, 4 }, shape{ 1, 1 }));
    CHECK(can_assign_from(shape{ 1, 4 }, shape{ 1 }));
    CHECK(can_assign_from(shape{ 1, 4 }, shape{}));

    CHECK(same_layout(shape{ 2, 3, 4 }, shape{ 2, 3, 4 }));
    CHECK(same_layout(shape{ 1, 2, 3, 4 }, shape{ 2, 3, 4 }));
    CHECK(same_layout(shape{ 2, 3, 4 }, shape{ 2, 1, 1, 3, 4 }));
    CHECK(same_layout(shape{ 2, 3, 4 }, shape{ 2, 3, 4, 1 }));
    CHECK(same_layout(shape{ 2, 1, 3, 4 }, shape{ 1, 2, 3, 4, 1 }));

    CHECK(!same_layout(shape{ 2, 1, 3, 4 }, shape{ 1, 2, 4, 3, 1 }));
    CHECK(!same_layout(shape{ 2, 1, 3, 4 }, shape{ 1, 2, 4, 3, 0 }));
}
} // namespace CMT_ARCH_NAME
} // namespace kfr
