export const initialState = {
  price: [0, 1000],
  availability: "All",
  color: "All",
  size: "All",
  activeFilterOnSale: false,
  brands: [],
  category: null,
  filtered: [],
  sortingOption: "Sort by (Default)",
  sorted: [],
  currentPage: 1,
  itemPerPage: 12,
  totalItems: 0,
  totalPages: 0,
  loading: false,
};

export function reducer(state, action) {
  switch (action.type) {
    case "SET_PRICE":
      return { ...state, price: action.payload };
    case "SET_COLOR":
      return { ...state, color: action.payload };
    case "SET_SIZE":
      return { ...state, size: action.payload };
    case "SET_AVAILABILITY":
      return { ...state, availability: action.payload };
    case "SET_BRANDS":
      return { ...state, brands: action.payload };
    case "SET_CATEGORY":
      return { ...state, category: action.payload, currentPage: 1 };
    case "SET_FILTERED":
      return { ...state, filtered: [...action.payload] };
    case "SET_SORTING_OPTION":
      return { ...state, sortingOption: action.payload };
    case "SET_SORTED":
      return { ...state, sorted: [...action.payload] };
    case "SET_CURRENT_PAGE":
      return { ...state, currentPage: action.payload };
    case "TOGGLE_FILTER_ON_SALE":
      return { ...state, activeFilterOnSale: !state.activeFilterOnSale, currentPage: 1 };
    case "SET_ITEM_PER_PAGE":
      return { ...state, itemPerPage: action.payload };
    case "SET_PRODUCTS":
      return {
        ...state,
        filtered: [...action.payload.products],
        sorted: [...action.payload.products],
        totalItems: action.payload.pagination.total,
        totalPages: action.payload.pagination.total_pages,
        loading: false,
      };
    case "SET_LOADING":
      return { ...state, loading: action.payload };
    case "CLEAR_FILTER":
      return {
        ...state,
        price: [0, 1000],
        availability: "All",
        color: "All",
        size: "All",
        brands: [],
        category: null,
        activeFilterOnSale: false,
        currentPage: 1,
      };
    default:
      return state;
  }
}
