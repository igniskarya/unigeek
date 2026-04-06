"use client";
const type = {
  MODAL: "MODAL",
};
const { MODAL } = type;

import { createContext, useCallback, useReducer } from "react";

const context = createContext();

const reducer = (state, action) => {
  const { type, payload } = action;
  switch (type) {
    case MODAL:
      return { ...state, modal: payload };
    default:
      return state;
  }
};

const State = (props) => {
  const initialState = { modal: false };
  const [state, dispatch] = useReducer(reducer, initialState);

  const modalToggle = useCallback((value) => {
    dispatch({ type: MODAL, payload: value });
  }, []);

  const { modal } = state;
  return (
    <context.Provider value={{ modal, modalToggle }}>
      {props.children}
    </context.Provider>
  );
};

export default State;
export { context };